#include "theme.h"

#include <QDir>
#include <QDirIterator>
#include <QGuiApplication>
#include <QPalette>
#include <QTemporaryDir>
#include <QtTest>

// The palette roles Theme::applyPalette() sets. Every one must come out
// valid for any theme that Theme reports as available — an invalid role is
// the "half-applied theme" failure mode.
static const QList<QPalette::ColorRole> kAppliedRoles = {
    QPalette::Window,        QPalette::WindowText,
    QPalette::Base,          QPalette::Text,
    QPalette::Button,        QPalette::ButtonText,
    QPalette::Highlight,     QPalette::HighlightedText,
    QPalette::Mid,           QPalette::Dark,
    QPalette::ToolTipBase,   QPalette::ToolTipText,
    QPalette::PlaceholderText,
};

class TestTheme : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void fullThemeApplies();
    void minimalThemeFallsBack();
    void incompleteThemeLeavesPaletteAlone();
    void installedOmarchyThemesApplyCleanly();

private:
    QString writeColors(const QByteArray &toml);

    QTemporaryDir m_dir;
};

QString TestTheme::writeColors(const QByteArray &toml)
{
    // A fresh subdirectory per call so each Theme sees its own file.
    static int counter = 0;
    const QString dir =
        m_dir.path() + QStringLiteral("/theme%1").arg(++counter);
    QDir().mkpath(dir);
    const QString path = dir + QStringLiteral("/colors.toml");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return {};
    file.write(toml);
    return path;
}

void TestTheme::init()
{
    // Theme snapshots the palette at construction and applies globally;
    // start every test from the stock palette.
    QGuiApplication::setPalette(QPalette());
}

void TestTheme::fullThemeApplies()
{
    const QString path = writeColors(
        "background = \"#1a1b26\"\n"
        "foreground = \"#a9b1d6\"\n"
        "accent = \"#7aa2f7\"\n"
        "selection = \"#292e42\"\n"
        "muted = \"#414868\"\n"
        "dark_background = \"#13141c\"\n"
        "darker_background = \"#0e0e14\"\n"
        "lighter_background = \"#24283b\"\n"
        "bright_foreground = \"#c0caf5\"\n");
    QVERIFY(!path.isEmpty());

    Theme theme(nullptr, path);
    QVERIFY(theme.available());

    const QPalette palette = QGuiApplication::palette();
    QCOMPARE(palette.color(QPalette::Window), QColor("#1a1b26"));
    QCOMPARE(palette.color(QPalette::WindowText), QColor("#a9b1d6"));
    QCOMPARE(palette.color(QPalette::Highlight), QColor("#292e42"));
    QCOMPARE(palette.color(QPalette::Base), QColor("#13141c"));
    for (const auto role : kAppliedRoles)
        QVERIFY(palette.color(role).isValid());
}

void TestTheme::minimalThemeFallsBack()
{
    // Only the two required keys: every optional role must fall back to a
    // valid color instead of an invalid QColor.
    const QString path = writeColors(
        "background = \"#101010\"\n"
        "foreground = \"#e0e0e0\"\n");
    QVERIFY(!path.isEmpty());

    Theme theme(nullptr, path);
    QVERIFY(theme.available());

    const QPalette palette = QGuiApplication::palette();
    for (const auto role : kAppliedRoles)
        QVERIFY2(palette.color(role).isValid(),
                 qPrintable(QStringLiteral("role %1 invalid").arg(role)));
    QCOMPARE(palette.color(QPalette::Highlight), QColor("#e0e0e0"));
    QCOMPARE(palette.color(QPalette::Base), QColor("#101010"));
}

void TestTheme::incompleteThemeLeavesPaletteAlone()
{
    const QPalette before = QGuiApplication::palette();
    const QString path = writeColors("background = \"#101010\"\n");
    QVERIFY(!path.isEmpty());

    Theme theme(nullptr, path);
    QVERIFY(!theme.available());
    QCOMPARE(QGuiApplication::palette(), before);
}

void TestTheme::installedOmarchyThemesApplyCleanly()
{
    // Sweep whatever Omarchy themes this machine ships. Assertions stay at
    // the app's actual contract (background+foreground required, everything
    // else falls back), so a legitimate future theme cannot fail this.
    const QString themesRoot = QStringLiteral("/usr/share/omarchy/themes");
    if (!QDir(themesRoot).exists())
        QSKIP("no Omarchy themes installed on this machine");

    int swept = 0;
    QDirIterator it(themesRoot, QDir::Dirs | QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        const QString colors = it.next() + QStringLiteral("/colors.toml");
        if (!QFile::exists(colors))
            continue;
        QGuiApplication::setPalette(QPalette());
        Theme theme(nullptr, colors);
        QVERIFY2(theme.available(), qPrintable(colors));
        const QPalette palette = QGuiApplication::palette();
        for (const auto role : kAppliedRoles)
            QVERIFY2(palette.color(role).isValid(), qPrintable(colors));
        ++swept;
    }
    QVERIFY(swept > 0);
    qInfo("swept %d installed themes", swept);
}

QTEST_MAIN(TestTheme)
#include "tst_theme.moc"
