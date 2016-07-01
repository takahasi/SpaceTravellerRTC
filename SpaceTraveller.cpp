// -*- C++ -*-
/*!
 * @file  SpaceTraveller.cpp * @brief Input component for SpaceTraveller of 3D Connexion  * $Date$ 
 *
 * $Id$ 
 */
#include "SpaceTraveller.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/joystick.h>

#define INPUT_DEVICE "/dev/input/js0"
#define THRESHOLD_H (15000)
#define THRESHOLD_L (-15000)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

//#define ENABLE_INFO
#define ENABLE_DEBUG
#define ENABLE_TRACE

#define _ERROR(fmt, ...) printf("[E]%s():"fmt, __func__, ## __VA_ARGS__)
#ifdef ENABLE_INFO
#define _INFO(fmt, ...) printf("[I]%s():"fmt, __func__, ## __VA_ARGS__)
#else
#define _INFO(fmt, ...)
#endif
#ifdef ENABLE_DEBUG
#define _DEBUG(fmt, ...) printf("[D]%s():"fmt, __func__, ## __VA_ARGS__)
#else
#define _DEBUG(fmt, ...)
#endif
#ifdef ENABLE_TRACE
#define _TRACE(fmt, ...) printf("[T]%s():"fmt, __func__, ## __VA_ARGS__)
#else
#define _TRACE(fmt, ...)
#endif

// Module specification
// <rtc-template block="module_spec">
static const char* spacetraveller_spec[] =
  {
    "implementation_id", "SpaceTraveller",
    "type_name",         "SpaceTraveller",
    "description",       "Input component for SpaceTraveller of 3D Connexion ",
    "version",           "1.0",
    "vendor",            "Takahashi",
    "category",          "example",
    "activity_type",     "SPORADIC",
    "kind",              "DataFlowComponent",
    "max_instance",      "10",
    "language",          "C++",
    "lang_type",         "compile",
    // Configuration variables
    ""
  };
// </rtc-template>

SpaceTraveller::SpaceTraveller(RTC::Manager* manager)
    // <rtc-template block="initializer">
  : RTC::DataFlowComponentBase(manager),
    m_outOut("out", m_out)
    // </rtc-template>
{
}

SpaceTraveller::~SpaceTraveller()
{
}


RTC::ReturnCode_t SpaceTraveller::onInitialize()
{
  // Registration: InPort/OutPort/Service
  // <rtc-template block="registration">
  // Set InPort buffers

  // Set OutPort buffer
  addOutPort("out", m_outOut);

  // Set service provider to Ports

  // Set service consumers to Ports

  // Set CORBA Service Ports

  // </rtc-template>

  // <rtc-template block="bind_config">
  // Bind variables and configuration variable

  // </rtc-template>

  return RTC::RTC_OK;
}


/*
RTC::ReturnCode_t SpaceTraveller::onFinalize()
{
  return RTC::RTC_OK;
}
*/
/*
RTC::ReturnCode_t SpaceTraveller::onStartup(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/
/*
RTC::ReturnCode_t SpaceTraveller::onShutdown(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

RTC::ReturnCode_t SpaceTraveller::onActivated(RTC::UniqueId ec_id)
{
    _TRACE("call %s\n", __FUNCTION__);

    m_out.data.length(6);

    m_task = new task();

    double p[6] = {0, 0, 0, 0, 0, 0};
    m_task->setPosition(p);
    m_task->enableExecute();
    m_task->activate();

    return RTC::RTC_OK;
}

RTC::ReturnCode_t SpaceTraveller::onDeactivated(RTC::UniqueId ec_id)
{
    _TRACE("call %s\n", __FUNCTION__);

    /* waits for finishing thread and free resources*/
    m_task->disableExecute();
    m_task->wait();
    m_task->finalize();
    delete m_task;

    return RTC::RTC_OK;
}

/* Avoids coflict name 'open' in coil::Task class */
static int WrapperOpen(const char *pathname, int flags)
{
    return open(pathname, flags);
}

int task::svc()
{
    struct js_event event;
    struct timeval tv;
    fd_set fds;
    int fd = -1;

    _TRACE("input thread start\n");

    /* try to open target input device */
    while (isEnableExecute() == 1) {
        /* open as input device */
        fd = WrapperOpen(INPUT_DEVICE, O_RDONLY);
        if (fd != -1) {
            break;
        }
        _ERROR("device open error! retry after 5 seconds\n");
        sleep(5);
    }

    while (isEnableExecute() == 1) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        if (select((fd + 1), &fds, NULL, NULL, &tv) < 0) {
            /* select errors */
            _ERROR("select fd error!");
            continue;
        }
        if (!FD_ISSET(fd, &fds)) {
            /* select timeout or caught unknown signals */
            _INFO("select timeout\n");
            continue;
        }
        /* read input js evant data from devfs */
        ssize_t len = sizeof(struct js_event);
        if (read(fd, &event, len) >= len) {
            if ((event.type & (~JS_EVENT_INIT)) == JS_EVENT_AXIS) {
                int update = 0;

                switch (event.number) {
                case 0: /* Pan L/R */
                case 1: /* Zoom */
                case 2: /* Pan U/D */
                case 3: /* Tilt */
                case 4: /* Roll */
                case 5: /* Spin */
                    update = 1;
                    break;
                default:
                    _ERROR("Unknown axis! : %d\n", event.number);
                    break;
                }

                if (update == 1) {
                    m_pos[event.number] = event.value;

                    const char *move_type[6][2] = {
                            {"Pan  Right"   ,   "Pan Left"  },
                            {"Zoom Left"    ,   "Zoom Right"},
                            {"Pan  Down"    ,   "Pan  Up"   },
                            {"Tilt Down"    ,   "Tilt Up"   },
                            {"Roll Left"    ,   "Roll Right"},
                            {"Spin Left"    ,   "Spin Right"}
                    };
                    if (event.value >= THRESHOLD_H) {
                        _DEBUG("%s\n", move_type[event.number][0]);
                    }
                    else if (event.value <= THRESHOLD_L) {
                        _DEBUG("%s\n", move_type[event.number][1]);
                    }
                    /* for debug print */
                    for (unsigned int i = 0; i < ARRAY_SIZE(m_pos); i++) {
                        _INFO("%f, ", m_pos[i]);
                    }
                    _INFO("\n");
                }
            }
        }
    }
    close(fd);
    _TRACE("input thread finish\n");
    return 0;
}

void task::setPosition(double *p)
{
    memcpy(m_pos, p, sizeof(double) * 6);
}

double* task::getPosition(void)
{
    return m_pos;
}

void task::enableExecute(void)
{
    m_alive = 1;
}

void task::disableExecute(void)
{
    m_alive = 0;
}

int task::isEnableExecute(void)
{
    return m_alive;
}

RTC::ReturnCode_t SpaceTraveller::onExecute(RTC::UniqueId ec_id)
{
    if (m_task) {
        if (m_task->isEnableExecute()) {
            /* "task alive" */
            memcpy(&(m_out.data[0]), m_task->getPosition(), sizeof(double) * m_out.data.length());
        }
        else {
            /* "task not alive" */
            memset(&(m_out.data[0]), 0, sizeof(double) * m_out.data.length());
        }
    }
    else {
        /* "task not exist" */
        memset(&(m_out.data[0]), 0, sizeof(double) * m_out.data.length());
    }

    /* for debug print */
    for (unsigned int i = 0; i < m_out.data.length(); i++) {
        _INFO("%f, ", m_out.data[i]);
    }
    _INFO("\n");

    m_outOut.write();

    return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t SpaceTraveller::onAborting(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/
/*
RTC::ReturnCode_t SpaceTraveller::onError(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/
/*
RTC::ReturnCode_t SpaceTraveller::onReset(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/
/*
RTC::ReturnCode_t SpaceTraveller::onStateUpdate(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/
/*
RTC::ReturnCode_t SpaceTraveller::onRateChanged(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/


extern "C"
{
 
  void SpaceTravellerInit(RTC::Manager* manager)
  {
    coil::Properties profile(spacetraveller_spec);
    manager->registerFactory(profile,
                             RTC::Create<SpaceTraveller>,
                             RTC::Delete<SpaceTraveller>);
  }
  
};



