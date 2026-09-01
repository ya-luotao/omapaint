#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QGuiApplication>
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
    app.setApplicationVersion(QStringLiteral("0.0.1"));

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
    parser.process(app);

    const QSize canvasSize =
        parseCanvasSize(parser.value(newOption), QSize(1280, 720));
    QUrl startupFile;
    if (!parser.positionalArguments().isEmpty())
        startupFile = QUrl::fromLocalFile(parser.positionalArguments().first());

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("startupFile"), startupFile},
        {QStringLiteral("startupSize"), canvasSize},
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

    return app.exec();
}
