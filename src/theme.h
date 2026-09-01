#pragma once

#include <QColor>
#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QPalette>
#include <QTimer>

// Applies the active Omarchy theme's colors.toml to the application palette,
// live-following theme switches. Fails gracefully: when the file is missing
// or lacks the required keys, the system palette is left completely alone —
// a half-applied theme is worse than none.
class Theme : public QObject
{
    Q_OBJECT

public:
    // colorsPath overrides the Omarchy staged-theme location (tests).
    explicit Theme(QObject *parent = nullptr,
                   const QString &colorsPath = QString());

    bool available() const { return m_available; }

private slots:
    void scheduleReload();
    void reload();

private:
    bool parseColors();
    void applyPalette();
    void rewatch();

    QString m_colorsPath;
    QHash<QString, QColor> m_colors;
    bool m_available = false;
    bool m_applied = false;
    QPalette m_systemPalette;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
};
