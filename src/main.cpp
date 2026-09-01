#include "canvas_item.h"
#include "demo_driver.h"
#include "theme.h"

#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QRegularExpression>

namespace {

QSize parseCanvasSize(const QString &spec, const QSize &fallback)
{
    static const QRegularExpression pattern(
        QStringLiteral("^(\\d+)x(\\d+)$"));
    const auto match = pattern.match(spec);
    if (!match.hasMatch())
        return fallback;
    return QSize(match.captured(1).toInt(), match.captured(2).toInt());
}

} // namespace

int main(int argc, char *argv[])
{
    QElapsedTimer startupTimer;
    startupTimer.start();

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("omapaint"));
    app.setApplicationDisplayName(QStringLiteral("OmaPaint"));
    app.setApplicationVersion(QStringLiteral(OMAPAINT_VERSION));
    app.setDesktopFileName(QStringLiteral("omapaint"));
    app.setWindowIcon(QIcon(QStringLiteral(":/omapaint.png")));

    // Follow the active Omarchy theme; leaves the system palette alone when
    // no Omarchy theme is present.
    Theme theme;

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Paint. For Omarchy."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("file"),
                                 QStringLiteral("Image file to open."));
    const QCommandLineOption newOption(
        QStringLiteral("new"),
        QStringLiteral("Start with a blank canvas of the given size."),
        QStringLiteral("WxH"));
    parser.addOption(newOption);
    const QCommandLineOption clipboardOption(
        QStringLiteral("clipboard"),
        QStringLiteral("Open the image currently on the clipboard."));
    parser.addOption(clipboardOption);
    const QCommandLineOption annotateOption(
        QStringLiteral("annotate"),
        QStringLiteral("Annotate mode: Done saves the file in place and "
                       "exits (used by omapaint-edit)."),
        QStringLiteral("file"));
    parser.addOption(annotateOption);
    QCommandLineOption demoOption(
        QStringLiteral("demo"),
        QStringLiteral("Run the scripted self-driving demo (promo recording)."));
    demoOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(demoOption);
    parser.process(app);

    const QSize canvasSize =
        parseCanvasSize(parser.value(newOption), QSize(1280, 720));
    QUrl startupFile;
    if (parser.isSet(annotateOption))
        startupFile = QUrl::fromLocalFile(parser.value(annotateOption));
    else if (!parser.positionalArguments().isEmpty())
        startupFile = QUrl::fromLocalFile(parser.positionalArguments().first());

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("startupFile"), startupFile},
        {QStringLiteral("startupSize"), canvasSize},
        // Wayland only exposes the clipboard to the focused window, so the
        // QML side defers the actual read until first activation.
        {QStringLiteral("startupClipboard"), parser.isSet(clipboardOption)},
        {QStringLiteral("annotateMode"), parser.isSet(annotateOption)},
    });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
                     [&startupTimer](QObject *object, const QUrl &) {
                         if (!object)
                             return;
                         qInfo("omapaint: engine loaded in %lld ms",
                               startupTimer.elapsed());
                         if (auto *window = qobject_cast<QQuickWindow *>(object)) {
                             auto connection =
                                 std::make_shared<QMetaObject::Connection>();
                             *connection = QObject::connect(
                                 window, &QQuickWindow::frameSwapped, window,
                                 [&startupTimer, connection] {
                                     qInfo("omapaint: first frame in %lld ms",
                                           startupTimer.elapsed());
                                     QObject::disconnect(*connection);
                                 });
                         }
                     });

    engine.loadFromModule("OmaPaint", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;

    if (parser.isSet(demoOption)) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        auto *canvas = window ? window->findChild<CanvasItem *>() : nullptr;
        if (window && canvas) {
            auto *driver = new DemoDriver(window, canvas, window);
            QObject::connect(driver, &DemoDriver::finished, &app,
                             &QCoreApplication::quit);
            driver->start();
        }
    }

    return app.exec();
}
