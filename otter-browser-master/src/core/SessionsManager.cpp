/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2021 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "SessionsManager.h"
#include "Application.h"
#include "JsonSettings.h"
#include "SessionModel.h"
#include "../ui/MainWindow.h"
#include "../ui/Window.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

namespace Otter
{

Session::Session(QObject *parent) : QObject(parent)
{
}

SessionsManager* SessionsManager::m_instance(nullptr);
SessionModel* SessionsManager::m_model(nullptr);
QString SessionsManager::m_sessionPath;
QString SessionsManager::m_sessionTitle;
QString SessionsManager::m_cachePath;
QString SessionsManager::m_profilePath;
QHash<QString, Session::Identity> SessionsManager::m_identities;
QVector<Session::MainWindow> SessionsManager::m_closedWindows;
bool SessionsManager::m_isDirty(false);
bool SessionsManager::m_isPrivate(false);
bool SessionsManager::m_isReadOnly(false);

SessionsManager::SessionsManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
}

void SessionsManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		m_isDirty = false;

		killTimer(m_saveTimer);

		m_saveTimer = 0;

		if (!m_isPrivate)
		{
			saveSession({}, {}, nullptr, false);
		}
	}
}

void SessionsManager::createInstance(const QString &profilePath, const QString &cachePath, bool isPrivate, bool isReadOnly)
{
	if (!m_instance)
	{
		m_instance = new SessionsManager(QCoreApplication::instance());
		m_cachePath = cachePath;
		m_profilePath = profilePath;
		m_isPrivate = isPrivate;
		m_isReadOnly = isReadOnly;
	}
}

void SessionsManager::scheduleSave()
{
	if (m_saveTimer == 0 && !m_isPrivate)
	{
		m_saveTimer = startTimer(1000);
	}
}

void SessionsManager::clearClosedWindows()
{
	m_closedWindows.clear();

	emit m_instance->closedWindowsChanged();
}

void SessionsManager::storeClosedWindow(MainWindow *mainWindow)
{
	if (!mainWindow || mainWindow->isPrivate())
	{
		return;
	}

	const Session::MainWindow session(mainWindow->getSession());

	if (session.windows.isEmpty())
	{
		return;
	}

	const int limit(SettingsManager::getOption(SettingsManager::History_ClosedTabsLimitAmountOption).toInt());

	m_closedWindows.prepend(session);

	if (m_closedWindows.count() > limit)
	{
		m_closedWindows.resize(limit);
		m_closedWindows.squeeze();
	}

	emit m_instance->closedWindowsChanged();
}

void SessionsManager::markSessionAsModified()
{
	if (!m_isPrivate && !m_isDirty && m_sessionPath == QLatin1String("default"))
	{
		m_isDirty = true;

		m_instance->scheduleSave();
	}
}

void SessionsManager::removeStoredUrl(const QString &url)
{
	emit m_instance->requestedRemoveStoredUrl(url);
}

SessionsManager* SessionsManager::getInstance()
{
	return m_instance;
}

SessionModel* SessionsManager::getModel()
{
	if (!m_model)
	{
		m_model = new SessionModel(m_instance);
	}

	return m_model;
}

QString SessionsManager::getCurrentSession()
{
	return m_sessionPath;
}

QString SessionsManager::getCachePath()
{
	return (m_isReadOnly ? QString() : m_cachePath);
}

QString SessionsManager::getProfilePath()
{
	return m_profilePath;
}

QString SessionsManager::getReadableDataPath(const QString &path, bool forceBundled)
{
	const QString writablePath(getWritableDataPath(path));

	return ((!forceBundled && QFile::exists(writablePath)) ? writablePath : QLatin1String(":/") + (path.contains(QLatin1Char('/')) ? QString() : QLatin1String("other")) + QDir::separator() + path);
}

QString SessionsManager::getWritableDataPath(const QString &path)
{
	return QDir::toNativeSeparators(m_profilePath + QDir::separator() + path);
}

QString SessionsManager::getSessionPath(const QString &path, bool isBound)
{
	QString normalizedPath(path);

	if (normalizedPath.isEmpty())
	{
		normalizedPath = QLatin1String("default.json");
	}
	else
	{
		if (!normalizedPath.endsWith(QLatin1String(".json")))
		{
			normalizedPath += QLatin1String(".json");
		}

		if (isBound)
		{
			normalizedPath = normalizedPath.replace(QLatin1Char('/'), QString()).replace(QLatin1Char('\\'), QString());
		}
		else if (QFileInfo(normalizedPath).isAbsolute())
		{
			return normalizedPath;
		}
	}

	return QDir::toNativeSeparators(m_profilePath + QLatin1String("/sessions/") + normalizedPath);
}

Session::Identity SessionsManager::getIdentity(const QString &name)
{
	return m_identities.value(name);
}

SessionInformation SessionsManager::getSession(const QString &path)
{
	SessionInformation session;
	const JsonSettings settings(getSessionPath(path));

	if (settings.isNull())
	{
		session.path = path;
		session.title = ((path == QLatin1String("default")) ? tr("Default") : tr("(Untitled)"));

		return session;
	}

	const int defaultZoom(SettingsManager::getOption(SettingsManager::Content_DefaultZoomOption).toInt());
	const QJsonArray mainWindowsArray(settings.object().value(QLatin1String("windows")).toArray());

	session.path = path;
	session.title = settings.object().value(QLatin1String("title")).toString((path == QLatin1String("default")) ? tr("Default") : tr("(Untitled)"));
	session.index = (settings.object().value(QLatin1String("currentIndex")).toInt(1) - 1);
	session.isClean = settings.object().value(QLatin1String("isClean")).toBool(true);

	for (int i = 0; i < mainWindowsArray.count(); ++i)
	{
		const QJsonObject mainWindowObject(mainWindowsArray.at(i).toObject());
		const QJsonArray windowsArray(mainWindowObject.value(QLatin1String("windows")).toArray());
		Session::MainWindow sessionMainWindow;
		sessionMainWindow.geometry = QByteArray::fromBase64(mainWindowObject.value(QLatin1String("geometry")).toString().toLatin1());
		sessionMainWindow.index = (mainWindowObject.value(QLatin1String("currentIndex")).toInt(1) - 1);

		for (int j = 0; j < windowsArray.count(); ++j)
		{
			const QJsonObject windowObject(windowsArray.at(j).toObject());
			const QJsonArray windowHistoryArray(windowObject.value(QLatin1String("history")).toArray());
			const QString state(windowObject.value(QLatin1String("state")).toString());
			Session::Window sessionWindow;
			sessionWindow.identity = windowObject.value(QLatin1String("identity")).toString();
			sessionWindow.state.geometry = JsonSettings::readRectangle(windowObject.value(QLatin1String("geometry")).toVariant());
			sessionWindow.state.state = ((state == QLatin1String("maximized")) ? Qt::WindowMaximized : ((state == QLatin1String("minimized")) ? Qt::WindowMinimized : Qt::WindowNoState));
			sessionWindow.history.index = (windowObject.value(QLatin1String("currentIndex")).toInt(1) - 1);
			sessionWindow.isAlwaysOnTop = windowObject.value(QLatin1String("isAlwaysOnTop")).toBool(false);
			sessionWindow.isPinned = windowObject.value(QLatin1String("isPinned")).toBool(false);

			if (windowObject.contains(QLatin1String("options")))
			{
				const QJsonObject optionsObject(windowObject.value(QLatin1String("options")).toObject());
				QJsonObject::const_iterator iterator;

				for (iterator = optionsObject.constBegin(); iterator != optionsObject.constEnd(); ++iterator)
				{
					const int optionIdentifier(SettingsManager::getOptionIdentifier(iterator.key()));

					if (optionIdentifier >= 0)
					{
						sessionWindow.options[optionIdentifier] = iterator.value().toVariant();
					}
				}
			}

			for (int k = 0; k < windowHistoryArray.count(); ++k)
			{
				const QJsonObject historyEntryObject(windowHistoryArray.at(k).toObject());
				const QStringList position(historyEntryObject.value(QLatin1String("position")).toString().split(QLatin1Char(',')));
				Session::Window::History::Entry historyEntry;
				historyEntry.url = historyEntryObject.value(QLatin1String("url")).toString();
				historyEntry.title = historyEntryObject.value(QLatin1String("title")).toString();
				historyEntry.position = ((position.count() == 2) ? QPoint(position.at(0).simplified().toInt(), position.at(1).simplified().toInt()) : QPoint(0, 0));
				historyEntry.zoom = historyEntryObject.value(QLatin1String("zoom")).toInt(defaultZoom);

				sessionWindow.history.entries.append(historyEntry);
			}

			if (sessionWindow.history.index < 0 || sessionWindow.history.index >= sessionWindow.history.entries.count())
			{
				sessionWindow.history.index = (sessionWindow.history.entries.count() - 1);
			}

			sessionMainWindow.windows.append(sessionWindow);
		}

		if (sessionMainWindow.index < 0 || sessionMainWindow.index >= sessionMainWindow.windows.count())
		{
			sessionMainWindow.index = (sessionMainWindow.windows.count() - 1);
		}

		if (mainWindowObject.contains(QLatin1String("splitters")))
		{
			const QJsonArray splittersArray(mainWindowObject.value(QLatin1String("splitters")).toArray());

			for (int j = 0; j < splittersArray.count(); ++j)
			{
				const QJsonObject splitterObject(splittersArray.at(j).toObject());
				const QVariantList rawSizes(splitterObject.value(QLatin1String("sizes")).toVariant().toList());
				QVector<int> sizes;
				sizes.reserve(rawSizes.count());

				for (int k = 0; k < rawSizes.count(); ++k)
				{
					sizes.append(rawSizes.at(k).toInt());
				}

				sessionMainWindow.splitters[splitterObject.value(QLatin1String("identifier")).toString()] = sizes;
			}
		}

		if (mainWindowObject.contains(QLatin1String("toolBars")))
		{
			const QJsonArray toolBarsArray(mainWindowObject.value(QLatin1String("toolBars")).toArray());
			QStringList toolBarsIdentifiers;
			toolBarsIdentifiers.reserve(toolBarsArray.count());

			sessionMainWindow.hasToolBarsState = true;
			sessionMainWindow.toolBars.reserve(toolBarsArray.count());

			for (int j = 0; j < toolBarsArray.count(); ++j)
			{
				const QJsonObject toolBarObject(toolBarsArray.at(j).toObject());
				const QString toolBarIdentifier(toolBarObject.value(QLatin1String("identifier")).toString());
				Session::MainWindow::ToolBarState toolBarState;
				toolBarState.identifier = ToolBarsManager::getToolBarIdentifier(toolBarIdentifier);

				if (toolBarsIdentifiers.contains(toolBarIdentifier))
				{
					continue;
				}

				toolBarsIdentifiers.append(toolBarIdentifier);

				if (toolBarObject.contains(QLatin1String("location")))
				{
					const QString location(toolBarObject.value(QLatin1String("location")).toString());

					if (location == QLatin1String("top"))
					{
						toolBarState.location = Qt::TopToolBarArea;
					}
					else if (location == QLatin1String("bottom"))
					{
						toolBarState.location = Qt::BottomToolBarArea;
					}
					else if (location == QLatin1String("left"))
					{
						toolBarState.location = Qt::LeftToolBarArea;
					}
					else if (location == QLatin1String("right"))
					{
						toolBarState.location = Qt::RightToolBarArea;
					}
				}

				if (toolBarObject.contains(QLatin1String("normalVisibility")))
				{
					toolBarState.normalVisibility = ((toolBarObject.value(QLatin1String("normalVisibility")).toString() == QLatin1String("hidden")) ? Session::MainWindow::ToolBarState::AlwaysHiddenToolBar : Session::MainWindow::ToolBarState::AlwaysVisibleToolBar);
				}

				if (toolBarObject.contains(QLatin1String("fullScreenVisibility")))
				{
					toolBarState.fullScreenVisibility = ((toolBarObject.value(QLatin1String("fullScreenVisibility")).toString() == QLatin1String("hidden")) ? Session::MainWindow::ToolBarState::AlwaysHiddenToolBar : Session::MainWindow::ToolBarState::AlwaysVisibleToolBar);
				}

				if (toolBarObject.contains(QLatin1String("row")))
				{
					toolBarState.row = toolBarObject.value(QLatin1String("row")).toInt(-1);
				}

				sessionMainWindow.toolBars.append(toolBarState);
			}

			sessionMainWindow.toolBars.squeeze();
		}

		session.windows.append(sessionMainWindow);
	}

	if (session.index < 0 || session.index >= session.windows.count())
	{
		session.index = (session.windows.count() - 1);
	}

	return session;
}

QStringList SessionsManager::getClosedWindows()
{
	QStringList closedWindows;
	closedWindows.reserve(m_closedWindows.count());

	for (int i = 0; i < m_closedWindows.count(); ++i)
	{
		const Session::MainWindow &window(m_closedWindows.at(i));
		const QString title(window.windows.value(window.index, Session::Window()).getTitle());

		closedWindows.append(title.isEmpty() ? tr("(Untitled)") : title);
	}

	return closedWindows;
}

QStringList SessionsManager::getSessions()
{
	const QList<QFileInfo> entries(QDir(m_profilePath + QLatin1String("/sessions/")).entryInfoList({QLatin1String("*.json")}, QDir::Files));
	QStringList sessions;
	sessions.reserve(entries.count());

	for (int i = 0; i < entries.count(); ++i)
	{
		sessions.append(entries.at(i).completeBaseName());
	}

	if (!m_sessionPath.isEmpty() && !entries.contains(m_sessionPath))
	{
		sessions.append(m_sessionPath);
	}

	if (!sessions.contains(QLatin1String("default")))
	{
		sessions.append(QLatin1String("default"));
	}

	sessions.sort();

	return sessions;
}

QVector<Session::Identity> SessionsManager::getIdentities()
{
	return m_identities.values().toVector();
}

SessionsManager::OpenHints SessionsManager::calculateOpenHints(OpenHints hints, Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
{
	const bool useNewTab(!hints.testFlag(NewWindowOpen) && SettingsManager::getOption(SettingsManager::Browser_OpenLinksInNewTabOption).toBool());

	if (button == Qt::MiddleButton && modifiers.testFlag(Qt::AltModifier))
	{
		return ((useNewTab ? NewTabOpen : NewWindowOpen) | BackgroundOpen | EndOpen);
	}

	if (modifiers.testFlag(Qt::ControlModifier) || button == Qt::MiddleButton)
	{
		return ((useNewTab ? NewTabOpen : NewWindowOpen) | BackgroundOpen);
	}

	if (modifiers.testFlag(Qt::ShiftModifier))
	{
		return (useNewTab ? NewTabOpen : NewWindowOpen);
	}

	if (hints.testFlag(NewTabOpen) && !hints.testFlag(NewWindowOpen))
	{
		return (useNewTab ? NewTabOpen : NewWindowOpen);
	}

	if (SettingsManager::getOption(SettingsManager::Browser_ReuseCurrentTabOption).toBool())
	{
		return CurrentTabOpen;
	}

	return hints;
}

SessionsManager::OpenHints SessionsManager::calculateOpenHints(OpenHints hints, Qt::MouseButton button)
{
	return calculateOpenHints(hints, button, QGuiApplication::keyboardModifiers());
}

SessionsManager::OpenHints SessionsManager::calculateOpenHints(const QVariantMap &parameters, bool ignoreModifiers)
{
	if (!parameters.contains(QLatin1String("hints")))
	{
		return calculateOpenHints(DefaultOpen, Qt::LeftButton, (ignoreModifiers ? Qt::NoModifier : QGuiApplication::keyboardModifiers()));
	}

	const QVariant::Type type(parameters[QLatin1String("hints")].type());

	if (type == QVariant::Int || type == QVariant::UInt)
	{
		return static_cast<OpenHints>(parameters[QLatin1String("hints")].toInt());
	}

	if (type != QVariant::List && type != QVariant::StringList)
	{
		return DefaultOpen;
	}

	const QStringList rawHints(parameters[QLatin1String("hints")].toStringList());
	OpenHints hints(DefaultOpen);

	for (int i = 0; i < rawHints.count(); ++i)
	{
		QString hint(rawHints.at(i));
		hint[0] = hint[0].toUpper();

		if (hint == QLatin1String("Private"))
		{
			hints |= PrivateOpen;
		}
		else if (hint == QLatin1String("CurrentTab"))
		{
			hints |= CurrentTabOpen;
		}
		else if (hint == QLatin1String("NewTab"))
		{
			hints |= NewTabOpen;
		}
		else if (hint == QLatin1String("NewWindow"))
		{
			hints |= NewWindowOpen;
		}
		else if (hint == QLatin1String("Background"))
		{
			hints |= BackgroundOpen;
		}
		else if (hint == QLatin1String("End"))
		{
			hints |= EndOpen;
		}
	}

	return hints;
}

bool SessionsManager::restoreClosedWindow(int index)
{
	if (index < 0 || index >= m_closedWindows.count())
	{
		return false;
	}

	Application::createWindow({}, m_closedWindows.takeAt(index));

	emit m_instance->closedWindowsChanged();

	return true;
}

bool SessionsManager::restoreSession(const SessionInformation &session, MainWindow *mainWindow, bool isPrivate)
{
	if (session.windows.isEmpty())
	{
		if (m_sessionPath.isEmpty() && session.path == QLatin1String("default"))
		{
			m_sessionPath = QLatin1String("default");
		}

		return false;
	}

	if (m_sessionPath.isEmpty())
	{
		m_sessionPath = session.path;
		m_sessionTitle = session.title;
	}

	const QVariantMap parameters({{QLatin1String("hints"), (isPrivate ? SessionsManager::PrivateOpen : SessionsManager::DefaultOpen)}});

	for (int i = 0; i < session.windows.count(); ++i)
	{
		if (mainWindow && i == 0)
		{
			mainWindow->restoreSession(session.windows.value(0));
		}
		else
		{
			Application::createWindow(parameters, session.windows.at(i));
		}
	}

	return true;
}

bool SessionsManager::saveSession(const QString &path, const QString &title, MainWindow *mainWindow, bool isClean)
{
	if (m_isPrivate && path.isEmpty())
	{
		return false;
	}

	SessionInformation session;
	session.path = getSessionPath(path);
	session.title = (title.isEmpty() ? m_sessionTitle : title);
	session.isClean = isClean;

	QVector<MainWindow*> windows;

	if (mainWindow)
	{
		windows.append(mainWindow);
	}
	else
	{
		windows = Application::getWindows();
	}

	session.windows.reserve(windows.count());

	for (int i = 0; i < windows.count(); ++i)
	{
		if (!windows.at(i)->isPrivate())
		{
			session.windows.append(windows.at(i)->getSession());
		}
	}

	session.windows.squeeze();

	return saveSession(session);
}

bool SessionsManager::saveSession(const SessionInformation &session)
{
	const QString sessionsPath(m_profilePath + QLatin1String("/sessions/"));

	QDir().mkpath(sessionsPath);

	if (session.windows.isEmpty())
	{
		return false;
	}

	QString path(session.path);

	if (path.isEmpty())
	{
		path = sessionsPath + session.title + QLatin1String(".json");

		if (QFile::exists(path))
		{
			int i(2);

			do
			{
				path = sessionsPath + session.title + QLatin1Char('_') + QString::number(i) + QLatin1String(".json");

				++i;
			}
			while (QFile::exists(path));
		}
	}

	const QStringList excludedOptions(SettingsManager::getOption(SettingsManager::Sessions_OptionsExludedFromSavingOption).toStringList());
	QJsonArray mainWindowsArray;
	QJsonObject sessionObject({{QLatin1String("title"), session.title}, {QLatin1String("currentIndex"), 1}});

	if (!session.isClean)
	{
		sessionObject.insert(QLatin1String("isClean"), false);
	}

	for (int i = 0; i < session.windows.count(); ++i)
	{
		const Session::MainWindow sessionEntry(session.windows.at(i));
		QJsonObject mainWindowObject({{QLatin1String("currentIndex"), (sessionEntry.index + 1)}, {QLatin1String("geometry"), QString::fromLatin1(sessionEntry.geometry.toBase64())}});
		QJsonArray windowsArray;

		for (int j = 0; j < sessionEntry.windows.count(); ++j)
		{
			QJsonObject windowObject({{QLatin1String("currentIndex"), (sessionEntry.windows.at(j).history.index + 1)}});

			if (!sessionEntry.windows.at(j).identity.isEmpty())
			{
				windowObject.insert(QLatin1String("identity"), sessionEntry.windows.at(j).identity);
			}

			if (!sessionEntry.windows.at(j).options.isEmpty())
			{
				const QHash<int, QVariant> windowOptions(sessionEntry.windows.at(j).options);
				QHash<int, QVariant>::const_iterator optionsIterator;
				QJsonObject optionsObject;

				for (optionsIterator = windowOptions.constBegin(); optionsIterator != windowOptions.constEnd(); ++optionsIterator)
				{
					const QString optionName(SettingsManager::getOptionName(optionsIterator.key()));

					if (!optionName.isEmpty() && !excludedOptions.contains(optionName))
					{
						optionsObject.insert(optionName, QJsonValue::fromVariant(optionsIterator.value()));
					}
				}

				windowObject.insert(QLatin1String("options"), optionsObject);
			}

			switch (sessionEntry.windows.at(j).state.state)
			{
				case Qt::WindowMaximized:
					windowObject.insert(QLatin1String("state"), QLatin1String("maximized"));

					break;
				case Qt::WindowMinimized:
					windowObject.insert(QLatin1String("state"), QLatin1String("minimized"));

					break;
				default:
					{
						const QRect geometry(sessionEntry.windows.at(j).state.geometry);

						windowObject.insert(QLatin1String("state"), QLatin1String("normal"));

						if (geometry.isValid())
						{
							windowObject.insert(QLatin1String("geometry"), QStringLiteral("%1, %2, %3, %4").arg(geometry.x()).arg(geometry.y()).arg(geometry.width()).arg(geometry.height()));
						}
					}

					break;
			}

			if (sessionEntry.windows.at(j).isAlwaysOnTop)
			{
				windowObject.insert(QLatin1String("isAlwaysOnTop"), true);
			}

			if (sessionEntry.windows.at(j).isPinned)
			{
				windowObject.insert(QLatin1String("isPinned"), true);
			}

			const Session::Window::History windowHistory(sessionEntry.windows.at(j).history);
			QJsonArray windowHistoryArray;

			for (int k = 0; k < windowHistory.entries.count(); ++k)
			{
				const QPoint position(windowHistory.entries.at(k).position);
				QJsonObject historyEntryObject({{QLatin1String("url"), windowHistory.entries.at(k).url}, {QLatin1String("title"), windowHistory.entries.at(k).title}, {QLatin1String("zoom"), windowHistory.entries.at(k).zoom}});

				if (!position.isNull())
				{
					historyEntryObject.insert(QLatin1String("position"), QStringLiteral("%1, %2").arg(position.x()).arg(position.y()));
				}

				windowHistoryArray.append(historyEntryObject);
			}

			windowObject.insert(QLatin1String("history"), windowHistoryArray);

			windowsArray.append(windowObject);
		}

		mainWindowObject.insert(QLatin1String("windows"), windowsArray);

		if (sessionEntry.hasToolBarsState)
		{
			QJsonArray toolBarsArray;

			for (int j = 0; j < sessionEntry.toolBars.count(); ++j)
			{
				const QString identifier(ToolBarsManager::getToolBarName(sessionEntry.toolBars.at(j).identifier));

				if (identifier.isEmpty())
				{
					continue;
				}

				QJsonObject toolBarObject({{QLatin1String("identifier"), identifier}});
				QString location;

				switch (sessionEntry.toolBars.at(j).location)
				{
					case Qt::LeftToolBarArea:
						location = QLatin1String("left");

						break;
					case Qt::RightToolBarArea:
						location = QLatin1String("right");

						break;
					case Qt::TopToolBarArea:
						location = QLatin1String("top");

						break;
					case Qt::BottomToolBarArea:
						location = QLatin1String("bottom");

						break;
					default:
						break;
				}

				if (!location.isEmpty())
				{
					toolBarObject.insert(QLatin1String("location"), location);
				}

				if (sessionEntry.toolBars.at(j).normalVisibility != Session::MainWindow::ToolBarState::UnspecifiedVisibilityToolBar)
				{
					toolBarObject.insert(QLatin1String("normalVisibility"), ((sessionEntry.toolBars.at(j).normalVisibility == Session::MainWindow::ToolBarState::AlwaysHiddenToolBar) ? QLatin1String("hidden") : QLatin1String("visible")));
				}

				if (sessionEntry.toolBars.at(j).fullScreenVisibility != Session::MainWindow::ToolBarState::UnspecifiedVisibilityToolBar)
				{
					toolBarObject.insert(QLatin1String("fullScreenVisibility"), ((sessionEntry.toolBars.at(j).fullScreenVisibility == Session::MainWindow::ToolBarState::AlwaysHiddenToolBar) ? QLatin1String("hidden") : QLatin1String("visible")));
				}

				if (sessionEntry.toolBars.at(j).row >= 0)
				{
					toolBarObject.insert(QLatin1String("row"), sessionEntry.toolBars.at(j).row);
				}

				toolBarsArray.append(toolBarObject);
			}

			mainWindowObject.insert(QLatin1String("toolBars"), toolBarsArray);
		}

		if (!sessionEntry.splitters.isEmpty())
		{
			QJsonArray splittersArray;
			QMap<QString, QVector<int> >::const_iterator iterator;

			for (iterator = sessionEntry.splitters.begin(); iterator != sessionEntry.splitters.end(); ++iterator)
			{
				QJsonArray sizesArray;
				const QVector<int> &sizes(iterator.value());

				for (int j = 0; j < sizes.count(); ++j)
				{
					sizesArray.append(sizes.at(j));
				}

				splittersArray.append(QJsonObject({{QLatin1String("identifier"), iterator.key()}, {QLatin1String("sizes"), sizesArray}}));
			}

			mainWindowObject.insert(QLatin1String("splitters"), splittersArray);
		}

		mainWindowsArray.append(mainWindowObject);
	}

	sessionObject.insert(QLatin1String("windows"), mainWindowsArray);

	JsonSettings settings;
	settings.setObject(sessionObject);

	return settings.save(path);
}

bool SessionsManager::deleteSession(const QString &path)
{
	const QString normalizedPath(getSessionPath(path, true));

	if (QFile::exists(normalizedPath))
	{
		return QFile::remove(normalizedPath);
	}

	return false;
}

bool SessionsManager::isPrivate()
{
	return m_isPrivate;
}

bool SessionsManager::isReadOnly()
{
	return m_isReadOnly;
}

bool SessionsManager::hasUrl(const QUrl &url, bool activate)
{
	MainWindow *activeWindow(Application::getActiveWindow());

	if (activeWindow ->hasUrl(url, activate))
	{
		Application::triggerAction(ActionsManager::ActivateWindowAction, {{QLatin1String("window"), activeWindow->getIdentifier()}}, m_instance);

		return true;
	}

	const QVector<MainWindow*> windows(Application::getWindows());
	QMultiMap<qint64, MainWindow*> map;

	for (int i = 0; i < windows.count(); ++i)
	{
		if (windows.at(i) != activeWindow && windows.at(i)->getActiveWindow())
		{
			map.insert(windows.at(i)->getActiveWindow()->getLastActivity().toMSecsSinceEpoch(), windows.at(i));
		}
	}

	const QVector<MainWindow*> sortedWindows(map.values().toVector());

	for (int i = (sortedWindows.count() - 1); i >= 0; --i)
	{
		if (sortedWindows.at(i)->hasUrl(url, activate))
		{
			Application::triggerAction(ActionsManager::ActivateWindowAction, {{QLatin1String("window"), sortedWindows.at(i)->getIdentifier()}}, m_instance);

			return true;
		}
	}

	return false;
}

}
