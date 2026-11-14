#include "APWindow.h"
#include <QToolButton>
#include <QSizeGrip>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QTextDocument>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <algorithm>
#include "APConnector/APMessages.h"

MiniAPWindow::MiniAPWindow(QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint)
{
    setWindowTitle("Mini AP Client");
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(280, 160);
    resize(460, 300);

    QFont messageFont("Segoe UI");
    messageFont.setPixelSize(14);
    setFont(messageFont);

    lockButton = new QToolButton(this);
    lockButton->setText("Lock");
    lockButton->setCheckable(true);
    lockButton->setFixedWidth(80);
    lockButton->setStyleSheet(
        "QToolButton { "
            "color: #cbd5e1; "
            "background: rgba(20,28,40,170); "
            "border: 1px solid rgba(180,205,230,45); "
            "border-radius: 4px; "
            "padding: 4px 12px; "
        "} "
        "QToolButton:hover { "
            "background: rgba(50,68,88,200); "
        "}"
    );

    QHBoxLayout* toolbar = new QHBoxLayout;
    toolbar->addStretch();
    toolbar->addWidget(lockButton);

    resizeHandle = new QSizeGrip(this);

    QVBoxLayout* windowLayout = new QVBoxLayout(this);
    windowLayout->setContentsMargins(12, 8, 12, 8);
    windowLayout->addLayout(toolbar);
    windowLayout->addStretch();
    windowLayout->addWidget(resizeHandle, 0, Qt::AlignRight);

    connect(
        lockButton, &QToolButton::toggled,
        this, &MiniAPWindow::changeLock
    );

    QTimer* messageTimer = new QTimer(this);

    connect(messageTimer, &QTimer::timeout, this, [this] {
        for (const std::string& message : Plugins::APC::takeAPMessages())
            receiveMessage(
                QByteArray(message.data(), message.size())
            );
    });

    messageTimer->start(30);
}

void MiniAPWindow::receiveMessage(const QByteArray& messageBytes)
{
    QJsonDocument document = QJsonDocument::fromJson(messageBytes);
    if (document.isObject()) {
        QJsonObject message = document.object();
        if (message["text"].isString()) addMessage(message["text"].toString());
        else if (message["parts"].isArray()) {
            QString formattedMessage;
            for (const QJsonValue& value : message["parts"].toArray()) {
                QJsonObject part = value.toObject();
                QString text = part["text"].toString().toHtmlEscaped().replace("\n", "<br>");
                QColor color(part["color"].toString());
                if (color.isValid())
                    text = QString("<span style='color:%1'>%2</span>").arg(color.name(), text);
                formattedMessage += text;
            }
            addFormattedMessage(formattedMessage);
        }
    }
}

void MiniAPWindow::addMessage(const QString& message)
{
    addFormattedMessage(message.toHtmlEscaped().replace("\n", "<br>"));
}

QString MiniAPWindow::coloredPlayerName(const QString& playerName)
{
    return QString("<span style='color:#9cdcff'>%1</span>").arg(playerName.toHtmlEscaped());
}

QString MiniAPWindow::coloredItemName(const QString& itemName, ItemClassification classification)
{
    QString itemColor = "#00eeee";
    if (classification == ItemClassification::Progression)
        itemColor = "#af99ef";
    else if (classification == ItemClassification::Useful)
        itemColor = "#6d8be8";
    return QString("<span style='color:%1'>%2</span>").arg(itemColor, itemName.toHtmlEscaped());
}

void MiniAPWindow::addItemMessage(const QString& senderName, const QString& itemName, const QString& recipientName, ItemClassification classification)
{
    QString message = QString("%1 sent %2 to %3.")
        .arg(coloredPlayerName(senderName), coloredItemName(itemName, classification), coloredPlayerName(recipientName));
    addFormattedMessage(message);
}

void MiniAPWindow::addHintMessage(const QString& recipientName, const QString& itemName, const QString& locationName,
    const QString& worldPlayerName, ItemClassification classification, bool wasFound)
{
    QString status = wasFound ? "Found" : "Not&nbsp;found";
    QString statusColor = wasFound ? "#87d7a0" : "#bac4d0";
    QString message = QString("[Hint] %1's %2 is at <span style='color:#9cdda5'>%3</span> in %4's world. <span style='color:%5'>[%6]</span>")
        .arg(coloredPlayerName(recipientName), coloredItemName(itemName, classification), locationName.toHtmlEscaped(),
            coloredPlayerName(worldPlayerName), statusColor, status);
    addFormattedMessage(message);
}

void MiniAPWindow::addFormattedMessage(const QString& message)
{
    QMetaObject::invokeMethod(this, [this, message] {
        messages.append(message);
        if (messages.size() > 6) messages.removeFirst();
        QTextDocument history;
        history.setHtml(messages.join("<br>"));
        setAccessibleDescription(history.toPlainText());
        update();
    }, Qt::AutoConnection);
}

void MiniAPWindow::changeLock(bool isLocked)
{
    if (isLocked)
        setFixedSize(size());
    else {
        setMinimumSize(280, 160);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
    lockButton->setText(isLocked ? "Unlock" : "Lock");
    resizeHandle->setVisible(!isLocked);
}

void MiniAPWindow::mousePressEvent(QMouseEvent* event)
{
    if (!lockButton->isChecked() && event->button() == Qt::LeftButton)
        dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
}

void MiniAPWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (!lockButton->isChecked() && (event->buttons() & Qt::LeftButton))
        move(event->globalPosition().toPoint() - dragOffset);
}

void MiniAPWindow::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 1));
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QColor(170, 197, 220, 210));
    painter.drawText(QRect(16, 12, width() - 120, 24), Qt::AlignVCenter, "AP  /  MESSAGE HISTORY");
    painter.setClipRect(QRect(0, 48, width(), height() - 66));
    int messageBottom = height() - 20;
    for (int olderMessages = 0; olderMessages < messages.size(); olderMessages++) {
        QTextDocument message;
        message.setDocumentMargin(0);
        message.setDefaultFont(font());
        message.setHtml("<span style='color:#ebf1f8'>" + messages[messages.size() - 1 - olderMessages] + "</span>");
        message.setTextWidth(width() - 56);
        int messageHeight = int(message.size().height()) + 12;
        QRect messageBounds(12, messageBottom - messageHeight, width() - 24, messageHeight);
        painter.setOpacity(std::max(0.15, 1.0 - olderMessages * 0.18));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(14, 22, 34, 155));
        painter.drawRoundedRect(messageBounds, 5, 5);
        painter.setPen(QColor(129, 194, 224, 160));
        painter.drawLine(messageBounds.topLeft() + QPoint(0, 7), messageBounds.bottomLeft() - QPoint(0, 7));
        painter.setPen(QColor(235, 241, 248));
        painter.save();
        painter.translate(messageBounds.left() + 16, messageBounds.top() + 6);
        message.drawContents(&painter);
        painter.restore();
        messageBottom = messageBounds.top();
    }
}
