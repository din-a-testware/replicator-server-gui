#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QtWebSockets/QWebSocketServer"
#include "QtWebSockets/QWebSocket"
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>
#include "db_search.h"
#include <QNetworkInterface>
#include <QTextStream>
#include <QDataStream>
#include <iostream>
#include <QStandardPaths>
#include <QDir>

MainWindow::MainWindow(quint16 port, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

      QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

      QFontDatabase LCARS_fonts;
      LCARS_fonts.addApplicationFont(":/fonts/Helvetica_Ultra_Compressed.ttf");

    //db.setDatabaseName(QDir::homePath()+"/replicator-data/databases/replicator_server_settings.db");
    db.setDatabaseName("/sdcard/replicator-data/databases/replicator_server_settings.db");

    if(db.open()){
        QSqlQuery query("SELECT `ip`,`port`,`protocol`,`db_file`,`db_path` FROM `settings` WHERE `id` = '0'");
         if (query.lastError().isValid()) {};
         query.exec();

         while (query.next()) {

             portServer=query.value(1).toInt();
             ui->debug_message->appendPlainText("Listening on port: " + QString::number(portServer));

             dbfile=query.value(4).toString()+query.value(3).toString();
             //qDebug() << "test" << dbfile;
             ui->debug_message->appendPlainText("Using database file: " + dbfile);

             }
    }
    else{};


    db.close();


    /*QString homeLocation = QStandardPaths::locate(QStandardPaths::HomeLocation, QString(), QStandardPaths::LocateDirectory);
qDebug() << homeLocation;
QString homepath = homeLocation;
    ui->debug_message->appendPlainText("downloc" + QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    if (QDir("/storage/emulated/0/replicator-data").exists()) {
        ui->debug_message->appendPlainText("/storage/emulated/0/replicator-data");
    }
    if (QFile("/sdcard/replicator-data/databases/replicator_server_settings.db").exists()) {
            ui->debug_message->appendPlainText("/sdcard/replicator-data/databases/replicator_server_settings.db");
    } else {
        ui->debug_message->appendPlainText("nix");
    }
*/

}

MainWindow::~MainWindow()
{
    m_pWebSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
    delete ui;
}

void MainWindow::on_button_StartServer_clicked()
{
    //quint16 portServer = ui->port_Edit->text().toUShort();

    m_pWebSocketServer = new QWebSocketServer(QStringLiteral("Replicator Server"),
                                              QWebSocketServer::NonSecureMode,
                                              this);
    if (m_pWebSocketServer->listen(QHostAddress::Any, portServer))
    {

        //qDebug() << "Replicator database server listening on port" << portServer;
        ui->debug_message->appendPlainText("Replicator database server listening on port" + portServer);


        //qDebug() << QCoreApplication::applicationPid();

        ui->debug_message->appendPlainText(QString::number(QCoreApplication::applicationPid()));

        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &MainWindow::onNewConnection);

        connect(m_pWebSocketServer, &QWebSocketServer::sslErrors,
                this, &MainWindow::onSslErrors);
    }
}

void MainWindow::onNewConnection(){
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    //qDebug() << "Client connected:" << pSocket->peerName() << pSocket->origin();
    ui->debug_message->appendPlainText("Client connected:" + pSocket->peerName() + pSocket->origin());
    connect(pSocket, &QWebSocket::textMessageReceived, this, &MainWindow::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived,
            this, &MainWindow::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &MainWindow::socketDisconnected);
    pSocket->sendTextMessage("Access to Library Computer Acces and Retrieval System granted!");
    m_clients << pSocket;
}
void MainWindow::processTextMessage(QString message){

    ui->debug_message->appendPlainText(message);

    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient)
    {
        QStringList messageSplitted = message.split("::");
        if (messageSplitted.at(0) == "SYSTEM") {
           //qDebug() << "system message recieved";
           //qDebug() << messageSplitted.at(1);
           QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
            //db.setDatabaseName(QDir::homePath()+"/replicator-data/databases/replicator_server_settings.db");
           db.setDatabaseName("/sdcard/replicator-data/databases/replicator_server_settings.db");
            if(db.open()){
                QSqlQuery query(messageSplitted.at(1));
                if (query.lastError().isValid()) {} else {ui->debug_message->appendPlainText(query.lastError().text());/*qDebug() << query.lastError().text();*/};
                 query.exec();
            } else { ui->debug_message->appendPlainText(db.lastError().text());
                /*qDebug() << "fehler" << db.lastError().text();*/ }
            db.close();
       }
        else if (messageSplitted.at(0)=="ping") {
            QString appPID = QString::number(QCoreApplication::applicationPid());
            pClient->sendTextMessage("pid:"+appPID);
        } else if (messageSplitted.at(0)=="shutdown") {
            std::cout << "Shutting down server. Closing all connections." << std::endl << "Goodbye! Live long and Prosper!";
            QCoreApplication::quit();
        }
        else if (messageSplitted.at(0)=="MARKED"){
            QVector<QStringList> returnSQL;

            QStringList returnType;
                    returnType.append("MARKED");
            returnSQL.append(returnType);
            returnSQL.append(db_Search->get_Data(dbfile,messageSplitted.at(1)));
            pClient->sendBinaryMessage(serialize(returnSQL));
        }
        else if (messageSplitted.at(0)=="NOTES"){
            QVector<QStringList> returnSQL;

            QStringList returnType;
                    returnType.append("NOTES");
            returnSQL.append(returnType);
            returnSQL.append(db_Search->get_Data(dbfile,messageSplitted.at(1)));
            pClient->sendBinaryMessage(serialize(returnSQL));
        }
        else if (messageSplitted.at(0)=="RECIPE_LIST") {
            QVector<QStringList> returnSQL;

            QStringList returnType;
            returnType.append("RECIPE_LIST");
            returnSQL.append(returnType);
            returnSQL.append(db_Search->get_Data(dbfile,messageSplitted.at(1)));
            pClient->sendBinaryMessage(serialize(returnSQL));
        }

        else if (messageSplitted.at(0)=="RECIPE") {
            QVector<QStringList> returnSQL;

            QStringList returnType;
            returnType.append("RECIPE");
            returnSQL.append(returnType);
            returnSQL.append(db_Search->get_Data(dbfile,messageSplitted.at(1)));
            pClient->sendBinaryMessage(serialize(returnSQL));
        } else if (messageSplitted.at(0)=="UPDATE") {
            db_Search->get_Data(dbfile,messageSplitted.at(1));
        }

    }

}
void MainWindow::processBinaryMessage(QByteArray message){



}
void MainWindow::socketDisconnected(){
    //qDebug() << "Client disconnected";
    ui->debug_message->appendPlainText("Client disconnected");

    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient)
    {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}
void MainWindow::onSslErrors(const QList<QSslError> &errors){

    //qDebug() << "Ssl errors occurred";
    ui->debug_message->appendPlainText("SSL errors occurred");

}
QByteArray MainWindow::serialize(QVector<QStringList> data){

    QByteArray byteArray;
    QDataStream out(&byteArray, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_5);
    out << data;
    return byteArray;

}
QVector<QStringList> MainWindow::deserialize(const QByteArray& byteArray){

    QVector<QStringList> data;
    QDataStream stream(byteArray);
        stream.setVersion(QDataStream::Qt_4_5);

        stream >> data;
        return data;

}
void MainWindow::createActions(){}
void MainWindow::createMenus(QString iface){}

void MainWindow::on_button_Close_clicked()
{
    this->close();
}

void MainWindow::on_button_StopServer_clicked()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient)
    {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}
