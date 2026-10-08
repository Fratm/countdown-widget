// Countdown Widget — a tiny KDE/Wayland desktop countdown.
//
// Shows "## Days until <Name>" in a small panel on the primary screen's
// desktop layer. The date and name are asked for once and saved to
// ~/.config/CountdownWidget/countdown.ini. The only control is a close (×)
// button. Colours and fonts come from the active Plasma theme.
//
// Run with --reset to clear the saved countdown and be prompted again.

#include <QApplication>
#include <QCommandLineParser>
#include <QDate>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include <QToolButton>
#include <QWidget>
#include <QWindow>

#ifdef HAVE_LAYERSHELLQT
#if LAYERSHELLQT_VERSION_MAJOR < 6
#include <LayerShellQt/Shell>
#endif
#include <LayerShellQt/Window>
#endif

static const char *kOrg = "CountdownWidget";
static const char *kApp = "countdown";

// ---------------------------------------------------------------------------
// First-run prompt: MM/DD/YYYY date + countdown name
// ---------------------------------------------------------------------------
static bool promptForCountdown(QDate &date, QString &name)
{
    QDialog dlg;
    dlg.setWindowTitle(QStringLiteral("New Countdown"));

    auto *dateEdit = new QDateEdit(QDate::currentDate().addDays(30), &dlg);
    dateEdit->setDisplayFormat(QStringLiteral("MM/dd/yyyy"));
    dateEdit->setCalendarPopup(true);

    auto *nameEdit = new QLineEdit(&dlg);
    nameEdit->setPlaceholderText(QStringLiteral("e.g. Vacation"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, [&] {
        if (nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, dlg.windowTitle(), QStringLiteral("Please enter a countdown name."));
            return;
        }
        dlg.accept();
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    auto *form = new QFormLayout(&dlg);
    form->addRow(QStringLiteral("Date (MM/DD/YYYY):"), dateEdit);
    form->addRow(QStringLiteral("Countdown name:"), nameEdit);
    form->addRow(buttons);

    if (dlg.exec() != QDialog::Accepted)
        return false;
    date = dateEdit->date();
    name = nameEdit->text().trimmed();
    return true;
}

// ---------------------------------------------------------------------------
// The widget itself
// ---------------------------------------------------------------------------
class CountdownWidget : public QWidget
{
public:
    CountdownWidget(QDate target, QString name)
        : m_target(target), m_name(std::move(name))
    {
        setWindowTitle(QStringLiteral("Countdown"));
        setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnBottomHint);
        setAttribute(Qt::WA_TranslucentBackground);

        m_label = new QLabel(this);
        QFont f = m_label->font();
        f.setPointSizeF(f.pointSizeF() * 1.4);
        f.setBold(true);
        m_label->setFont(f);

        auto *close = new QToolButton(this);
        close->setText(QStringLiteral("×"));
        close->setToolTip(QStringLiteral("Close countdown"));
        close->setAutoRaise(true);
        close->setCursor(Qt::PointingHandCursor);
        connect(close, &QToolButton::clicked, qApp, &QApplication::quit);

        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(14, 8, 6, 8);
        lay->setSpacing(10);
        lay->addWidget(m_label);
        lay->addWidget(close, 0, Qt::AlignTop);

        refresh();

        // Re-check every minute so the count rolls over at midnight.
        auto *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &CountdownWidget::refresh);
        timer->start(60 * 1000);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        // Rounded card in the theme's window colour, slightly translucent.
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QColor bg = palette().color(QPalette::Window);
        bg.setAlpha(225);
        QColor border = palette().color(QPalette::Mid);
        border.setAlpha(160);
        QPainterPath path;
        path.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 10, 10);
        p.fillPath(path, bg);
        p.setPen(border);
        p.drawPath(path);
    }

    void changeEvent(QEvent *e) override
    {
        // Repaint when the user switches between light/dark themes.
        if (e->type() == QEvent::PaletteChange || e->type() == QEvent::ApplicationPaletteChange)
            update();
        QWidget::changeEvent(e);
    }

private:
    void refresh()
    {
        const qint64 days = QDate::currentDate().daysTo(m_target);
        QString text;
        if (days > 0)
            text = QStringLiteral("%1 %2 until %3").arg(days).arg(days == 1 ? "Day" : "Days", m_name);
        else if (days == 0)
            text = QStringLiteral("Today: %1!").arg(m_name);
        else
            text = QStringLiteral("%1 %2 since %3").arg(-days).arg(days == -1 ? "Day" : "Days", m_name);
        if (m_label->text() != text) {
            m_label->setText(text);
            adjustSize();
        }
    }

    QDate m_target;
    QString m_name;
    QLabel *m_label = nullptr;
};

// ---------------------------------------------------------------------------
// Place the widget on the desktop layer of the primary screen.
// ---------------------------------------------------------------------------
static void placeOnDesktop(CountdownWidget &w, QSettings &settings)
{
    const QString corner = settings.value("Position/corner", "top-right").toString();
    const int margin = settings.value("Position/margin", 48).toInt();
    QScreen *screen = QGuiApplication::primaryScreen();

#ifdef HAVE_LAYERSHELLQT
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) {
        w.winId(); // create the native window so we can configure it before showing
        if (screen)
            w.windowHandle()->setScreen(screen); // Plasma 6 places the layer on this screen
        if (auto *lw = LayerShellQt::Window::get(w.windowHandle())) {
            using LW = LayerShellQt::Window;
            LW::Anchors anchors;
            anchors |= corner.startsWith("bottom") ? LW::AnchorBottom : LW::AnchorTop;
            anchors |= corner.endsWith("left") ? LW::AnchorLeft : LW::AnchorRight;
            lw->setAnchors(anchors);
            lw->setMargins(QMargins(margin, margin, margin, margin));
            lw->setLayer(LW::LayerBottom);           // above wallpaper, below windows
            lw->setExclusiveZone(-1);                // don't push panels around
            lw->setKeyboardInteractivity(LW::KeyboardInteractivityNone);
            lw->setScope(QStringLiteral("countdown-widget"));
#if LAYERSHELLQT_VERSION_MAJOR < 6
            if (screen)
                lw->setDesiredOutput(screen); // Plasma 5 only; removed in Plasma 6
#endif
            return;
        }
    }
#endif
    // Fallback (X11, or built without layer-shell): plain positioning.
    if (!screen)
        return;
    w.adjustSize();
    const QRect g = screen->availableGeometry();
    const int x = corner.endsWith("left") ? g.left() + margin : g.right() - margin - w.width();
    const int y = corner.startsWith("bottom") ? g.bottom() - margin - w.height() : g.top() + margin;
    w.move(x, y);
}

int main(int argc, char *argv[])
{
#ifdef HAVE_LAYERSHELLQT
#if LAYERSHELLQT_VERSION_MAJOR < 6
    // Plasma 5's layer-shell-qt must be enabled before QApplication exists.
    LayerShellQt::Shell::useLayerShell();
#endif
#endif
    QApplication app(argc, argv);
    QApplication::setOrganizationName(kOrg);
    QApplication::setApplicationName(kApp);
    QApplication::setDesktopFileName(QStringLiteral("countdown-widget"));
    QApplication::setQuitOnLastWindowClosed(true);

    QCommandLineParser parser;
    parser.setApplicationDescription("Desktop countdown widget");
    parser.addHelpOption();
    QCommandLineOption resetOpt("reset", "Forget the saved countdown and ask again.");
    parser.addOption(resetOpt);
    parser.process(app);

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, kOrg, kApp);
    if (parser.isSet(resetOpt)) {
        settings.remove("Countdown");
        settings.sync();
    }

    QDate date = QDate::fromString(settings.value("Countdown/date").toString(), Qt::ISODate);
    QString name = settings.value("Countdown/name").toString();

    if (!date.isValid() || name.isEmpty()) {
        if (!promptForCountdown(date, name))
            return 0;
        settings.setValue("Countdown/date", date.toString(Qt::ISODate));
        settings.setValue("Countdown/name", name);
        if (!settings.contains("Position/corner"))
            settings.setValue("Position/corner", "top-right");
        if (!settings.contains("Position/margin"))
            settings.setValue("Position/margin", 48);
        settings.sync();
    }

    CountdownWidget w(date, name);
    placeOnDesktop(w, settings);
    w.show();
    return app.exec();
}
