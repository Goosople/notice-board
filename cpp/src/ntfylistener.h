#pragma once

#include <QString>

namespace NtfySender {

bool send(const QString &server, const QString &topic, const QString &message,
          const QString &title = "Notice Board", int priority = 0,
          const QString &click = "", const QString &tags = "");

}
