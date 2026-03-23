#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTimer>
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

    QCommandLineOption pageOpt({"p", "page"},
        "Apri direttamente alla pagina N", "N");
    parser.addOption(pageOpt);
    parser.process(app);

    MainWindow win;
    win.show();

    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        win.openFile(args.first());
        if (parser.isSet(pageOpt)) {
            int page = parser.value(pageOpt).toInt();
            if (page > 0)
                QTimer::singleShot(500, &win, [&win, page]{
                    win.scrollToPage(page - 1);
                });
        }
    }

    return app.exec();
}
