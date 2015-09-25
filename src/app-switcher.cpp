#include <QDebug>
#include <QApplication>
#include <QDesktopWidget>
#include <LXQtGlobalKeys/Action>
#include <LXQtGlobalKeys/Client>
#include <QKeyEvent>
#include <QTimer>
#include <kwindowsystem.h>

#include "app-switcher.h"
#include "app-model.h"
#include "app-itemdelegate.h"
#include "settings.h"

AppSwitcher::AppSwitcher(QWidget *parent):
    QListView(parent)
{
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint);
    setObjectName("AppSwitcher");
    //QString shortcut = "Control+Next";
    QString shortcut = "Alt+Tab";
    m_globalShortcut = GlobalKeyShortcut::Client::instance()->addAction(shortcut, "/app_switcher/switch", tr("Switch applications"), this);

    connect(m_globalShortcut, &GlobalKeyShortcut::Action::activated, [this]{
        if (isVisible())
            selectNextItem();
        else
            showSwitcher();
    });

    connect(this, &AppSwitcher::activated, [](const QModelIndex& index){
        qDebug() << index.row();
    });

    setItemDelegate(new AppItemDelegate(this));
    setContentsMargins(5, 5, 5, 5);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_timer = new QTimer();
    m_timer->setInterval(100);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &AppSwitcher::timer);
}


void AppSwitcher::showSwitcher()
{
    m_timer->start();
    Settings::instance().sync();
    setModel(new AppModel(this));
    dynamic_cast<AppItemDelegate*>(itemDelegate())->init();

    m_current = 0;

    int w = 0;
    int h = 0;
    int maxApp = Settings::instance().maxDisplayApps();

    for(int i = 0; i < model()->rowCount(); ++i){
        QSize siz = itemDelegate()->sizeHint(QStyleOptionViewItem(), model()->index(i, 0));
        w = qMax(w, siz.width());
        h += siz.height();
        if (i > maxApp)
            break;
    }

    resize(w+contentsMargins().left()+contentsMargins().right(), h+contentsMargins().top()+contentsMargins().bottom());

    QRect active = QApplication::desktop()->screenGeometry(QCursor::pos());
    move(active.left()+active.width()/2 - width() / 2, active.top()+active.height()/2 - height() / 2);
    selectNextItem();
    show();
}

void AppSwitcher::selectNextItem()
{
    m_timer->start();
    if(++m_current >= model()->rowCount())
        m_current = 0;
    setCurrentIndex(model()->index(m_current, 0));
    setFocus();
}

void AppSwitcher::currentChanged(const QModelIndex &/*current*/, const QModelIndex &/*previous*/)
{
    //qDebug() << "current" << current.row();
}

void AppSwitcher::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
}

void AppSwitcher::keyReleaseEvent(QKeyEvent *event)
{
    if (event->modifiers() == 0){
        m_timer->stop();
        KWindowSystem::activateWindow(model()->data(model()->index(m_current, 0), AppRole::Window).value<WId>());
        close();
    }
    QWidget::keyReleaseEvent(event);
}

void AppSwitcher::timer()
{
    if (QApplication::queryKeyboardModifiers() == Qt::NoModifier){
        KWindowSystem::activateWindow(model()->data(model()->index(m_current, 0), AppRole::Window).value<WId>());
        close();
    }
}