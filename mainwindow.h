#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QByteArray>
#include <QtNetwork/QSslError>
#include "db_search.h"
#include <QAction>


QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

class db_search;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(quint16 port,QWidget *parent = 0);
    virtual ~MainWindow();

    db_search *db_Search = new db_search;
    QString dbfile;

    QVector<QStringList> localQuery(QString queryString);

     QAction *exitAct;
     quint16 portServer;

private slots:
    void on_button_StartServer_clicked();

private Q_SLOTS:
    void onNewConnection();
    void processTextMessage(QString message);
    void processBinaryMessage(QByteArray message);
    void socketDisconnected();
    void onSslErrors(const QList<QSslError> &errors);
    QByteArray serialize(QVector<QStringList> data);
    QVector<QStringList> deserialize(const QByteArray& byteArray);
    void createActions();
    void createMenus(QString iface);

    void on_button_Close_clicked();

    void on_button_StopServer_clicked();

private:
    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
