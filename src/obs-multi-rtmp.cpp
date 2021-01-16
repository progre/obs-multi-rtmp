﻿#include "pch.h"

#include <list>
#include <regex>

#include "push-widget.h"

#define ConfigSection "obs-multi-rtmp"

QThread* g_uiThread = nullptr;


class MultiOutputWidget : public QDockWidget
{
    int dockLocation_;
    bool dockVisible_;
    bool reopenShown_;

public:
    MultiOutputWidget(QWidget* parent = 0)
        : QDockWidget(parent)
        , reopenShown_(false)
    {
        setWindowTitle(obs_module_text("Title"));
        setFeatures((DockWidgetFeatures)(AllDockWidgetFeatures & ~DockWidgetClosable));

        // save dock location
        QObject::connect(this, &QDockWidget::dockLocationChanged, [this](Qt::DockWidgetArea area) {
            dockLocation_ = area;
            
        });

        scroll_ = new QScrollArea(this);
        scroll_->move(0, 22);

        container_ = new QWidget(this);
        layout_ = new QGridLayout(container_);
        layout_->setAlignment(Qt::AlignmentFlag::AlignTop);

        // init widget
        addButton_ = new QPushButton(obs_module_text("Btn.NewTarget"), container_);
        QObject::connect(addButton_, &QPushButton::clicked, [this]() {
            auto pushwidget = createPushWidget(QJsonObject(), container_);
            layout_->addWidget(pushwidget);
            if (!pushwidget->ShowEditDlg())
                delete pushwidget;
        });
        layout_->addWidget(addButton_);

        if (std::string(u8"多路推流") == obs_module_text("Title"))
            layout_->addWidget(new QLabel(u8"本插件免费提供，请不要为此付费。\n作者：雷鸣", container_));
        else
            layout_->addWidget(new QLabel(u8"This plugin provided free of charge. \nAuthor: SoraYuki", container_));

        // load config
        LoadConfig();

        scroll_->setWidgetResizable(true);
        scroll_->setWidget(container_);

        setLayout(layout_);

        resize(200, 400);
    }

    void visibleToggled(bool visible)
    {
        dockVisible_ = visible;
        return;

        if (visible == false
            && reopenShown_ == false
            && !config_has_user_value(obs_frontend_get_global_config(), ConfigSection, "DockVisible"))
        {
            reopenShown_ = true;
            QMessageBox(QMessageBox::Icon::Information, 
                obs_module_text("Notice.Title"), 
                obs_module_text("Notice.Reopen"), 
                QMessageBox::StandardButton::Ok,
                this).exec();
        }
    }

    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::Resize)
        {
            scroll_->resize(width(), height() - 22);
        }
        return QDockWidget::event(event);
    }

    std::vector<PushWidget*> GetAllPushWidgets()
    {
        std::vector<PushWidget*> result;
        for(auto& c : container_->children())
        {
            if (c->objectName() == "push-widget")
            {
                auto w = dynamic_cast<PushWidget*>(c);
                result.push_back(w);
            }
        }
        return result;
    }

    void StopAll()
    {
        for(auto x : GetAllPushWidgets())
            x->Stop();
    }

    void SaveConfig()
    {
        QJsonArray targetlist;
        for(auto x : GetAllPushWidgets())
            targetlist.append(x->Config());
        QJsonObject root;
        root["targets"] = targetlist;
        QJsonDocument jsondoc;
        jsondoc.setObject(root);
        config_set_string(obs_frontend_get_global_config(), ConfigSection, "json", jsondoc.toBinaryData().toBase64());

        config_set_int(obs_frontend_get_global_config(), ConfigSection, "DockLocation", (int)dockLocation_);
        config_set_bool(obs_frontend_get_global_config(), ConfigSection, "DockVisible", dockVisible_);
    }

    void LoadConfig()
    {
        QJsonObject conf;
        auto base64str = config_get_string(obs_frontend_get_global_config(), ConfigSection, "json");
        if (base64str && *base64str)
        {
            auto bindat = QByteArray::fromBase64(base64str);
            auto jsondoc = QJsonDocument::fromBinaryData(bindat);
            if (jsondoc.isObject())
                conf = jsondoc.object();
        }

        auto targets = conf.find("targets");
        if (targets != conf.end() && targets->isArray())
        {
            for(auto x : targets->toArray())
            {
                if (x.isObject())
                {
                    auto pushwidget = createPushWidget(((QJsonValue)x).toObject(), container_);
                    layout_->addWidget(pushwidget);
                }
            }
        }
    }

private:
    QWidget* container_ = 0;
    QScrollArea* scroll_ = 0;
    QGridLayout* layout_ = 0;
    QPushButton* addButton_ = 0;
};



OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-multi-rtmp", "en-US")
OBS_MODULE_AUTHOR("雷鳴 (@sorayukinoyume)")

bool obs_module_load()
{
    if (obs_get_version() < MAKE_SEMANTIC_VERSION(25, 0, 0))
        return false;
    
    auto mainwin = (QMainWindow*)obs_frontend_get_main_window();
    if (mainwin == nullptr)
        return false;
    QMetaObject::invokeMethod(mainwin, []() {
        g_uiThread = QThread::currentThread();
    });

    auto dock = new MultiOutputWidget(mainwin);
    auto action = (QAction*)obs_frontend_add_dock(dock);
    QObject::connect(action, &QAction::toggled, dock, &MultiOutputWidget::visibleToggled);

    // begin work around obs not remember dock geometry added by obs_frontend_add_dock
    mainwin->removeDockWidget(dock);
    auto docklocation = config_get_int(obs_frontend_get_global_config(), ConfigSection, "DockLocation");
    auto visible = config_get_bool(obs_frontend_get_global_config(), ConfigSection, "DockVisible");
    if (!config_has_user_value(obs_frontend_get_global_config(), ConfigSection, "DockLocation"))
    {
        docklocation = Qt::DockWidgetArea::LeftDockWidgetArea;
    }
    if (!config_has_user_value(obs_frontend_get_global_config(), ConfigSection, "DockVisible"))
    {
        visible = true;
    }

    mainwin->addDockWidget((Qt::DockWidgetArea)docklocation, dock);
    if (visible)
    {
        dock->setVisible(true);
        action->setChecked(true);
    }
    else
    {
        dock->setVisible(false);
        action->setChecked(false);
    }
    // end work around

    obs_frontend_add_event_callback(
        [](enum obs_frontend_event event, void *private_data) {
            if (event == obs_frontend_event::OBS_FRONTEND_EVENT_EXIT)
            {
                static_cast<MultiOutputWidget*>(private_data)->SaveConfig();
                static_cast<MultiOutputWidget*>(private_data)->StopAll();
            }
        }, dock
    );

    return true;
}

const char *obs_module_description(void)
{
    return "Multiple RTMP Output Plugin";
}