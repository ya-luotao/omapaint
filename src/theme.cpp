#include "theme.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QRegularExpression>

Theme::Theme(QObject *parent)
    : QObject(parent)
    , m_colorsPath(QDir::homePath()
                   + QStringLiteral("/.local/state/omarchy/current/theme/colors.toml"))
    , m_systemPalette(QGuiApplication::palette())
{
    // Theme switches rebuild the staged theme directory, which unhooks
    // watchers on the file itself — so watch the ancestors too and re-add
    // whatever exists after every change.
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(200);
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this,
            &Theme::scheduleReload);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this,
            &Theme::scheduleReload);

    rewatch();
    reload();
}

void Theme::scheduleReload()
{
    m_debounce.start();
}

void Theme::reload()
{
    rewatch();
    m_available = parseColors();
    applyPalette();
}

void Theme::rewatch()
{
    const QStringList watched = m_watcher.files() + m_watcher.directories();
    if (!watched.isEmpty())
        m_watcher.removePaths(watched);

    const QString current =
        QDir::homePath() + QStringLiteral("/.local/state/omarchy/current");
    for (const QString &path :
         {current, current + QStringLiteral("/theme"), m_colorsPath}) {
        if (QFile::exists(path))
            m_watcher.addPath(path);
    }
}

bool Theme::parseColors()
{
    QFile file(m_colorsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    m_colors.clear();
    static const QRegularExpression line(
        QStringLiteral("^\\s*([a-z_]+)\\s*=\\s*\"(#[0-9a-fA-F]{6,8})\""));
    while (!file.atEnd()) {
        const auto match = line.match(QString::fromUtf8(file.readLine()));
        if (match.hasMatch()) {
            const QColor color(match.captured(2));
            if (color.isValid())
                m_colors.insert(match.captured(1), color);
        }
    }

    return m_colors.contains(QStringLiteral("background"))
        && m_colors.contains(QStringLiteral("foreground"));
}

void Theme::applyPalette()
{
    if (!m_available) {
        if (m_applied) {
            QGuiApplication::setPalette(m_systemPalette);
            m_applied = false;
        }
        return;
    }

    const auto color = [this](const char *key, const char *fallback) {
        return m_colors.value(QLatin1String(key),
                              m_colors.value(QLatin1String(fallback)));
    };

    const QColor background = color("background", "background");
    const QColor foreground = color("foreground", "foreground");

    QPalette palette;
    palette.setColor(QPalette::Window, background);
    palette.setColor(QPalette::WindowText, foreground);
    palette.setColor(QPalette::Base, color("dark_background", "background"));
    palette.setColor(QPalette::Text, foreground);
    palette.setColor(QPalette::Button, color("lighter_background", "background"));
    palette.setColor(QPalette::ButtonText, foreground);
    palette.setColor(QPalette::Highlight, color("selection", "accent"));
    palette.setColor(QPalette::HighlightedText,
                     color("bright_foreground", "foreground"));
    palette.setColor(QPalette::Mid, color("muted", "foreground"));
    palette.setColor(QPalette::Dark, color("darker_background", "background"));
    palette.setColor(QPalette::ToolTipBase, color("dark_background", "background"));
    palette.setColor(QPalette::ToolTipText, foreground);
    palette.setColor(QPalette::PlaceholderText, color("muted", "foreground"));

    QColor disabled = foreground;
    disabled.setAlpha(110);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabled);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled);

    QGuiApplication::setPalette(palette);
    m_applied = true;
}
