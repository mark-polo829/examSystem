/****************************************************************************
** Meta object code from reading C++ file 'StudentMainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.14.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../client/StudentMainWindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StudentMainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.14.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_StudentMainWindow_t {
    QByteArrayData data[7];
    char stringdata0[110];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_StudentMainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_StudentMainWindow_t qt_meta_stringdata_StudentMainWindow = {
    {
QT_MOC_LITERAL(0, 0, 17), // "StudentMainWindow"
QT_MOC_LITERAL(1, 18, 18), // "startExamRequested"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 15), // "logoutRequested"
QT_MOC_LITERAL(4, 54, 18), // "onStartExamClicked"
QT_MOC_LITERAL(5, 73, 15), // "onLogoutClicked"
QT_MOC_LITERAL(6, 89, 20) // "onViewDetailsClicked"

    },
    "StudentMainWindow\0startExamRequested\0"
    "\0logoutRequested\0onStartExamClicked\0"
    "onLogoutClicked\0onViewDetailsClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_StudentMainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   39,    2, 0x06 /* Public */,
       3,    0,   40,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   41,    2, 0x08 /* Private */,
       5,    0,   42,    2, 0x08 /* Private */,
       6,    0,   43,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void StudentMainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<StudentMainWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->startExamRequested(); break;
        case 1: _t->logoutRequested(); break;
        case 2: _t->onStartExamClicked(); break;
        case 3: _t->onLogoutClicked(); break;
        case 4: _t->onViewDetailsClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (StudentMainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StudentMainWindow::startExamRequested)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (StudentMainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StudentMainWindow::logoutRequested)) {
                *result = 1;
                return;
            }
        }
    }
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject StudentMainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_StudentMainWindow.data,
    qt_meta_data_StudentMainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *StudentMainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StudentMainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_StudentMainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int StudentMainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void StudentMainWindow::startExamRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void StudentMainWindow::logoutRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
