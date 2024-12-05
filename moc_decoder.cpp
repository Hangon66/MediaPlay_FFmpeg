/****************************************************************************
** Meta object code from reading C++ file 'decoder.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../myFFmpeg/decoder.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'decoder.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Decoder_t {
    QByteArrayData data[17];
    char stringdata0[183];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Decoder_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Decoder_t qt_meta_stringdata_Decoder = {
    {
QT_MOC_LITERAL(0, 0, 7), // "Decoder"
QT_MOC_LITERAL(1, 8, 16), // "signal_ShowVideo"
QT_MOC_LITERAL(2, 25, 0), // ""
QT_MOC_LITERAL(3, 26, 3), // "img"
QT_MOC_LITERAL(4, 30, 19), // "signal_SendDuration"
QT_MOC_LITERAL(5, 50, 4), // "time"
QT_MOC_LITERAL(6, 55, 16), // "signal_SendState"
QT_MOC_LITERAL(7, 72, 18), // "Decoder::PlayState"
QT_MOC_LITERAL(8, 91, 5), // "state"
QT_MOC_LITERAL(9, 97, 11), // "decoderFile"
QT_MOC_LITERAL(10, 109, 4), // "file"
QT_MOC_LITERAL(11, 114, 4), // "type"
QT_MOC_LITERAL(12, 119, 9), // "slot_Seek"
QT_MOC_LITERAL(13, 129, 3), // "pos"
QT_MOC_LITERAL(14, 133, 18), // "slot_AudioFinished"
QT_MOC_LITERAL(15, 152, 14), // "slot_StopVideo"
QT_MOC_LITERAL(16, 167, 15) // "slot_PauseVideo"

    },
    "Decoder\0signal_ShowVideo\0\0img\0"
    "signal_SendDuration\0time\0signal_SendState\0"
    "Decoder::PlayState\0state\0decoderFile\0"
    "file\0type\0slot_Seek\0pos\0slot_AudioFinished\0"
    "slot_StopVideo\0slot_PauseVideo"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Decoder[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x06 /* Public */,
       4,    1,   57,    2, 0x06 /* Public */,
       6,    1,   60,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    2,   63,    2, 0x0a /* Public */,
      12,    1,   68,    2, 0x0a /* Public */,
      14,    0,   71,    2, 0x0a /* Public */,
      15,    0,   72,    2, 0x0a /* Public */,
      16,    0,   73,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QImage,    3,
    QMetaType::Void, QMetaType::LongLong,    5,
    QMetaType::Void, 0x80000000 | 7,    8,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   10,   11,
    QMetaType::Void, QMetaType::LongLong,   13,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void Decoder::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Decoder *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->signal_ShowVideo((*reinterpret_cast< QImage(*)>(_a[1]))); break;
        case 1: _t->signal_SendDuration((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 2: _t->signal_SendState((*reinterpret_cast< Decoder::PlayState(*)>(_a[1]))); break;
        case 3: _t->decoderFile((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 4: _t->slot_Seek((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 5: _t->slot_AudioFinished(); break;
        case 6: _t->slot_StopVideo(); break;
        case 7: _t->slot_PauseVideo(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Decoder::*)(QImage );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Decoder::signal_ShowVideo)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Decoder::*)(qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Decoder::signal_SendDuration)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Decoder::*)(Decoder::PlayState );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Decoder::signal_SendState)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Decoder::staticMetaObject = { {
    &QThread::staticMetaObject,
    qt_meta_stringdata_Decoder.data,
    qt_meta_data_Decoder,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Decoder::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Decoder::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Decoder.stringdata0))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int Decoder::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void Decoder::signal_ShowVideo(QImage _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Decoder::signal_SendDuration(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Decoder::signal_SendState(Decoder::PlayState _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
