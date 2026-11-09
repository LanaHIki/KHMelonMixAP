#pragma once
#include <QWidget>
#include <QStringList>

class QToolButton;
class QSizeGrip;

class MiniAPWindow : public QWidget
{
public:
    enum class ItemClassification { Progression, Useful, Filler };
    explicit MiniAPWindow(QWidget* parent = nullptr);
    void addMessage(const QString& message);
    void addItemMessage(const QString& senderName, const QString& itemName, const QString& recipientName, ItemClassification classification);
    void addHintMessage(const QString& recipientName, const QString& itemName, const QString& locationName,
        const QString& worldPlayerName, ItemClassification classification, bool wasFound);

private:
    void receiveMessage(const QByteArray& messageBytes);
    QString coloredPlayerName(const QString& playerName);
    QString coloredItemName(const QString& itemName, ItemClassification classification);
    void addFormattedMessage(const QString& message);
    void changeLock(bool isLocked);
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    QPoint dragOffset;
    QStringList messages;
    QToolButton* lockButton;
    QSizeGrip* resizeHandle;
};
