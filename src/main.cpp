#include <QApplication>
#include <QCommandLineParser>
#include "MainWindow.h"
#include "Theme.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("SFOGLI-ACCIO");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SfogliAccio");
    app.setStyleSheet(Theme::globalStyleSheet());

    QCommandLineParser parser;
    parser.setApplicationDescription("SFOGLI-ACCIO — lettore PDF veloce, moderno e gratuito");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "PDF da aprire (opzionale)");
    parser.process(app);

    MainWindow win;
    win.show();

    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty())
        win.openFile(args.first());

    return app.exec();
}
