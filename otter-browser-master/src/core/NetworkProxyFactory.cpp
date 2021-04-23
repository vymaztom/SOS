/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2019 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "NetworkProxyFactory.h"
#include "NetworkAutomaticProxy.h"

namespace Otter
{

NetworkProxyFactory::NetworkProxyFactory(QObject *parent) : QObject(parent),
	m_automaticProxy(nullptr),
	m_definition(ProxyDefinition())
{
}

NetworkProxyFactory::~NetworkProxyFactory()
{
	delete m_automaticProxy;
}

void NetworkProxyFactory::setProxy(const QString &identifier)
{
	m_definition = NetworkManagerFactory::getProxy(identifier);

	m_proxies.clear();
	m_proxies[-1] = {QNetworkProxy(QNetworkProxy::NoProxy)};

	switch (m_definition.type)
	{
		case ProxyDefinition::ManualProxy:
			{
				QHash<ProxyDefinition::ProtocolType, ProxyDefinition::ProxyServer>::iterator iterator;

				for (iterator = m_definition.servers.begin(); iterator != m_definition.servers.end(); ++iterator)
				{
					if (iterator.key() == ProxyDefinition::AnyProtocol)
					{
						const QVector<ProxyDefinition::ProtocolType> protocols({ProxyDefinition::HttpProtocol, ProxyDefinition::HttpsProtocol, ProxyDefinition::FtpProtocol});

						for (int i = 0; i < protocols.count(); ++i)
						{
							m_proxies[protocols.at(i)] = {QNetworkProxy(getProxyType(protocols.at(i)), iterator.value().hostName, iterator.value().port)};
						}
					}
					else
					{
						m_proxies[iterator.key()] = {QNetworkProxy(getProxyType(iterator.key()), iterator.value().hostName, iterator.value().port)};
					}
				}
			}

			break;
		case ProxyDefinition::AutomaticProxy:
			if (m_automaticProxy)
			{
				m_automaticProxy->setPath(m_definition.path);
			}
			else
			{
				m_automaticProxy = new NetworkAutomaticProxy(m_definition.path);
			}

			break;
		default:
			break;
	}
}

QList<QNetworkProxy> NetworkProxyFactory::queryProxy(const QNetworkProxyQuery &query)
{
	switch (m_definition.type)
	{
		case ProxyDefinition::SystemProxy:
			return QNetworkProxyFactory::systemProxyForQuery(query);

		case ProxyDefinition::ManualProxy:
			{
				const QString host(query.peerHostName());

				for (int i = 0; i < m_definition.exceptions.count(); ++i)
				{
					if (m_definition.exceptions.at(i).contains(QLatin1Char('/')))
					{
						const QHostAddress address(host);
						const QPair<QHostAddress, int> subnet(QHostAddress::parseSubnet(m_definition.exceptions.at(i)));

						if (!address.isNull() && subnet.second != -1 && address.isInSubnet(subnet))
						{
							return m_proxies[-1];
						}
					}
					else if (host.contains(m_definition.exceptions.at(i), Qt::CaseInsensitive))
					{
						return m_proxies[-1];
					}
				}

				if (m_proxies.contains(ProxyDefinition::SocksProtocol))
				{
					return m_proxies[ProxyDefinition::SocksProtocol];
				}

				const QString protocol(query.protocolTag().toLower());

				if (protocol == QLatin1String("http") && m_proxies.contains(ProxyDefinition::HttpProtocol))
				{
					return m_proxies[ProxyDefinition::HttpProtocol];
				}

				if (protocol == QLatin1String("https") && m_proxies.contains(ProxyDefinition::HttpsProtocol))
				{
					return m_proxies[ProxyDefinition::HttpsProtocol];
				}

				if (protocol == QLatin1String("ftp") && m_proxies.contains(ProxyDefinition::FtpProtocol))
				{
					return m_proxies[ProxyDefinition::FtpProtocol];
				}

				return m_proxies[-1];
			}
		case ProxyDefinition::AutomaticProxy:
			if (m_automaticProxy && m_automaticProxy->isValid())
			{
				return m_automaticProxy->getProxy(query.url().toString(), query.peerHostName()).toList();
			}

			return QNetworkProxyFactory::systemProxyForQuery(query);
		default:
			break;
	}

	return m_proxies[-1];
}

QNetworkProxy::ProxyType NetworkProxyFactory::getProxyType(ProxyDefinition::ProtocolType protocol)
{
	switch (protocol)
	{
		case ProxyDefinition::HttpProtocol:
		case ProxyDefinition::HttpsProtocol:
			return QNetworkProxy::HttpProxy;
		case ProxyDefinition::FtpProtocol:
			return QNetworkProxy::FtpCachingProxy;
		case ProxyDefinition::SocksProtocol:
			return QNetworkProxy::Socks5Proxy;
		default:
			break;
	}

	return QNetworkProxy::DefaultProxy;
}

bool NetworkProxyFactory::usesSystemAuthentication()
{
	return m_definition.usesSystemAuthentication;
}

}
