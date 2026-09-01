#include "canvas_item.h"
#include "demo_driver.h"
#include "document.h"
#include "image_io.h"
#include "ruby.h"
#include "theme.h"

#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickImageProvider>
#include <QQuickWindow>
#include <QRegularExpression>

#include <cstdio>
#include <unistd.h>

namespace {

// Serves "image://ruby/<fur-color>" to QML so the About dialog can show
// Ruby in the current palette without QML ever touching pixels.
class RubyImageProvider : public QQuickImageProvider
{
public:
    RubyImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Image)
    {
    }

    QImage requestImage(const QString &id, QSize *size,
                        const QSize &requestedSize) override
    {
        Q_UNUSED(requestedSize);
        QColor fur(u'#' + id);
        if (!fur.isValid())
            fur = QColor("#dcd7ba");
        const QImage image = Ruby::portrait(fur, QColor("#c34043"));
        if (size)
            *size = image.size();
        return image;
    }
};

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
    parser.setApplicationDescription(QStringLiteral(
        "Paint. For Omarchy.\n"
        "\n"
        "Examples:\n"
        "  omapaint image.png            edit an image (PNG, JPEG, WebP)\n"
        "  omapaint --new 1280x720       blank canvas of a given size\n"
        "  omapaint --clipboard          edit the image on the clipboard\n"
        "  wl-paste | omapaint -         same, via stdin\n"
        "  omapaint --annotate shot.png  annotate; Ctrl+Enter (Done) saves in\n"
        "                                place and exits 0, discard exits 1"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("file"),
                                 QStringLiteral("Image file to open, or - to "
                                                "read an image from stdin."));
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
    QImage stdinImage;
    if (parser.isSet(annotateOption)) {
        startupFile = QUrl::fromLocalFile(parser.value(annotateOption));
    } else if (!parser.positionalArguments().isEmpty()) {
        const QString arg = parser.positionalArguments().first();
        if (arg == QStringLiteral("-")) {
            // wl-paste | omapaint -
            if (isatty(fileno(stdin))) {
                fprintf(stderr, "omapaint: '-' expects image data on stdin "
                                "(e.g. wl-paste | omapaint -)\n");
                return 1;
            }
            QFile in;
            QString error;
            if (!in.open(stdin, QIODevice::ReadOnly)
                || !ImageIo::loadData(in.readAll(), &stdinImage, &error)) {
                fprintf(stderr, "omapaint: stdin is not a supported image: %s\n",
                        qPrintable(error));
                return 1;
            }
        } else {
            startupFile = QUrl::fromLocalFile(arg);
        }
    }

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("ruby"), new RubyImageProvider);
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

    if (!stdinImage.isNull()) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        if (auto *doc = window ? window->findChild<Document *>() : nullptr) {
            doc->newFromImage(stdinImage);
            qInfo("omapaint: opened %dx%d image from stdin",
                  doc->imageSize().width(), doc->imageSize().height());
        }
    }

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
