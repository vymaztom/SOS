Summary: pdf conversion, scaling and viewing by blocks of text
Name: pdftoroff
Version: 1.1.0
Release: 1
License: GPLv3
Group: Graphics
Url: https://github.com/sgerwk/pdftoroff
Source: pdftoroff-%{version}.tar.gz

# curl -L -o $HOME/rpmbuild/SOURCES/pdftoroff-1.0.0.tar.gz
# https://github.com/sgerwk/pdftoroff/archive/v1.0.0.tar.gz

%description
The pdftoroff program extracts text from pdf files, trying to
undo page, column and paragraph formatting while retaining italic
and bold faces. It ouputs text in various text formats: groff,
html, plain TeX, text, or user-defined format. It is used by the
included pdftoebook script to reformat a pdf file to a smaller
page, so that it becomes suited to be read on a small tablet or
e-ink ebook reader.

Also included is pdffit, which resizes the pages of a pdf file so
that their text fits into a given paper size (e.g., A4 or letter)
with a given margin. The related program pdfrects finds the
bounding box or the text area of the pages of a pdf file.

The hovacui pdf viewer for the Linux framebuffer and X11
automatically zooms to the blocks of text. It is aimed at viewing
files on small screens, and is especially handy for
multiple-columns documents.

%prep
%setup

%build
make

%install
make PREFIX=/usr DESTDIR=%{?buildroot} install

%clean
rm -rf %{buildroot}

%files
/usr/bin/pdftoroff
/usr/bin/pdffit
/usr/bin/pdfrects
/usr/bin/pdftoebook
/usr/bin/pdfannot
/usr/bin/hovacui
%doc /usr/share/man/man1/pdffit.1.gz
%doc /usr/share/man/man1/pdftoroff.1.gz
%doc /usr/share/man/man1/pdfrects.1.gz
%doc /usr/share/man/man1/hovacui.1.gz
