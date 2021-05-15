#include "mainwindow.h"
#include "ui_mainwindow.h"

//参考：
//・3Dグラフ表示　座標ファイルを読み込んで散布図(scatterとして表示)　（マウス操作：ズームあり、視点移動なし）
//https://github.com/fengfanchen/Qt/tree/master/3D%20move%20rotation
//
//・参考：Qt付属のExampleコード　customplot　（マウス操作：ズームあり、視点移動あり）

#include <QtDataVisualization/QScatter3DSeries>
#include <QDebug>
#include <QThread>
#include <QMessageBox>
#include <QHBoxLayout>

//-start- Qt_Official_sample:custominput
#include <QtDataVisualization/QScatterDataProxy>
#include <QtDataVisualization/QValue3DAxis>
#include <QtDataVisualization/Q3DScene>
#include <QtDataVisualization/Q3DCamera>
#include <QtDataVisualization/QScatter3DSeries>
#include <QtDataVisualization/Q3DTheme>
#include <QtCore/qmath.h>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>
//-end- Qt_Official_sample:custominput

//-start- Qt_Official_sample:bars
#include <QtDataVisualization/qcategory3daxis.h>
#include <QtDataVisualization/qvalue3daxis.h>
#include <QtDataVisualization/qbardataproxy.h>
#include <QtDataVisualization/q3dscene.h>
#include <QtDataVisualization/q3dcamera.h>
#include <QtDataVisualization/qbar3dseries.h>
#include <QtDataVisualization/q3dtheme.h>
#include <QtCore/QTime>
#include <QtWidgets/QComboBox>
#include <QtCore/qmath.h>
//-end- Qt_Official_sample:bars


using namespace QtDataVisualization;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_xRotation(0.0f)
    , m_yRotation(0.0f)
{
    ui->setupUi(this);
    this->setWindowTitle("3D scatterGraph . QtDataVisualization");
    func_GUIdefault(); //GUIフォーム初期化

    m_graph = new Q3DScatter();
    m_graph->axisX()->setRange(-10, 10);
    m_graph->axisY()->setRange(-10, 10);
    m_graph->axisZ()->setRange(-10, 10);
    QWidget *container = QWidget::createWindowContainer(m_graph);

    QScatter3DSeries *series = new QScatter3DSeries;
    series->setItemLabelFormat(QStringLiteral("@xLabel, @yLabel, @zLabel"));
    series->setMesh(QAbstract3DSeries::MeshCube);
    series->setItemSize(0.15f);
    m_graph->addSeries(series);

    if (!m_graph->hasContext()) {

        QMessageBox msgBox;
        msgBox.setText("Couldn't initialize the OpenGL context.");
        msgBox.exec();
        return;
    }

    addData(); //csvファイルからデータを読み込んで追加

    //设置到控件上　コントロールに設定→ Widgetに登録、画面で表示されるようになる
    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget(container, 1);
    ui->widget->setLayout(hLayout);

    //-start- 視点移動 Qt-Official-sample bars
    changePresetCamera();
    // Set up property animations for zooming to the selected bar
    //! [12]
    Q3DCamera *camera = m_graph->scene()->activeCamera();
    m_defaultAngleX = camera->xRotation();
    m_defaultAngleY = camera->yRotation();
    m_defaultZoom = camera->zoomLevel();
    m_defaultTarget = camera->target();

    m_animationCameraX.setTargetObject(camera);
    m_animationCameraY.setTargetObject(camera);
    m_animationCameraZoom.setTargetObject(camera);
    m_animationCameraTarget.setTargetObject(camera);

    m_animationCameraX.setPropertyName("xRotation");
    m_animationCameraY.setPropertyName("yRotation");
    m_animationCameraZoom.setPropertyName("zoomLevel");
    m_animationCameraTarget.setPropertyName("target");

    int duration = 1700;
    m_animationCameraX.setDuration(duration);
    m_animationCameraY.setDuration(duration);
    m_animationCameraZoom.setDuration(duration);
    m_animationCameraTarget.setDuration(duration);

    // The zoom always first zooms out above the graph and then zooms in
    qreal zoomOutFraction = 0.3;
    m_animationCameraX.setKeyValueAt(zoomOutFraction, QVariant::fromValue(0.0f));
    m_animationCameraY.setKeyValueAt(zoomOutFraction, QVariant::fromValue(90.0f));
    m_animationCameraZoom.setKeyValueAt(zoomOutFraction, QVariant::fromValue(50.0f));
    m_animationCameraTarget.setKeyValueAt(zoomOutFraction,
                                          QVariant::fromValue(QVector3D(0.0f, 0.0f, 0.0f)));
    //! [12]
    //-end- 視点移動 Qt-Official-sample bars


}

MainWindow::~MainWindow()
{
    qDebug() << "Widget::~Widget()";
    delete m_graph;
    delete ui;
}


void MainWindow::addData()
{
    QVector<QVector3D> itemList;

    // Read data items from the file to QVector
    QTextStream stream;
    QFile dataFile(":/data/data.txt");
    if (dataFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        stream.setDevice(&dataFile);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (line.startsWith("#")) // Ignore comments
                continue;
            QStringList strList = line.split(",", Qt::SkipEmptyParts);
            // Each line has three data items: xPos, yPos and zPos value
            if (strList.size() < 3) {
                qWarning() << "Invalid row read from data:" << line;
                continue;
            }
            itemList.append(QVector3D(
                                 strList.at(0).trimmed().toFloat(),
                                 strList.at(1).trimmed().toFloat(),
                                 strList.at(2).trimmed().toFloat()));
        }
    } else {
        qWarning() << "Unable to open data file:" << dataFile.fileName();
    }

    // Add data from the QVector to datamodel 行、データを生成する
    QScatterDataArray *dataArray = new QScatterDataArray;
    dataArray->resize(itemList.count());
    QScatterDataItem *ptrToDataArray = &dataArray->first();
    for (int i = 0; i < itemList.count(); i++) {
        ptrToDataArray->setPosition(itemList.at(i));
        ptrToDataArray++;
    }

    m_graph->seriesList().at(0)->dataProxy()->resetArray(dataArray);
}


//-start- kuroda 視点移動 Qt-official-sample bar より流用
void MainWindow::changePresetCamera()
{
    m_animationCameraX.stop();
    m_animationCameraY.stop();
    m_animationCameraZoom.stop();
    m_animationCameraTarget.stop();

    // Restore camera target in case animation has changed it
    m_graph->scene()->activeCamera()->setTarget(QVector3D(0.0f, 0.0f, 0.0f));

    //! [10]
    static int preset = Q3DCamera::CameraPresetFront;

    m_graph->scene()->activeCamera()->setCameraPreset((Q3DCamera::CameraPreset)preset);

    if (++preset > Q3DCamera::CameraPresetDirectlyBelow)
        preset = Q3DCamera::CameraPresetFrontLow;
    //! [10]
}

void MainWindow::on_rotationSliderX_valueChanged(int value)
{
    rotateX(value);
}

void MainWindow::on_rotationSliderY_valueChanged(int value)
{
    rotateY(value);
}

void MainWindow::rotateX(int rotation) //3D視点移動
{
    m_xRotation = rotation;
    m_graph->scene()->activeCamera()->setCameraPosition(m_xRotation, m_yRotation);
}

void MainWindow::rotateY(int rotation) //3D視点移動
{
    m_yRotation = rotation;
    m_graph->scene()->activeCamera()->setCameraPosition(m_xRotation, m_yRotation);
}

////! [11]
//void MainWindow::zoomToSelectedBar() //不要？　もとの別のズームの処理だけ？
//{
//    m_animationCameraX.stop();
//    m_animationCameraY.stop();
//    m_animationCameraZoom.stop();
//    m_animationCameraTarget.stop();

//    Q3DCamera *camera = m_graph->scene()->activeCamera();
//    float currentX = camera->xRotation();
//    float currentY = camera->yRotation();
//    float currentZoom = camera->zoomLevel();
//    QVector3D currentTarget = camera->target();

//    m_animationCameraX.setStartValue(QVariant::fromValue(currentX));
//    m_animationCameraY.setStartValue(QVariant::fromValue(currentY));
//    m_animationCameraZoom.setStartValue(QVariant::fromValue(currentZoom));
//    m_animationCameraTarget.setStartValue(QVariant::fromValue(currentTarget));

//    QPoint selectedBar = m_graph->selectedSeries()
//            ? m_graph->selectedSeries()->selectedBar()
//            : QBar3DSeries::invalidSelectionPosition();

//    if (selectedBar != QBar3DSeries::invalidSelectionPosition()) {
//        // Normalize selected bar position within axis range to determine target coordinates
//        //! [13]
//        QVector3D endTarget;
//        float xMin = m_graph->columnAxis()->min();
//        float xRange = m_graph->columnAxis()->max() - xMin;
//        float zMin = m_graph->rowAxis()->min();
//        float zRange = m_graph->rowAxis()->max() - zMin;
//        endTarget.setX((selectedBar.y() - xMin) / xRange * 2.0f - 1.0f);
//        endTarget.setZ((selectedBar.x() - zMin) / zRange * 2.0f - 1.0f);
//        //! [13]

//        // Rotate the camera so that it always points approximately to the graph center
//        //! [15]
//        qreal endAngleX = 90.0 - qRadiansToDegrees(qAtan(qreal(endTarget.z() / endTarget.x())));
//        if (endTarget.x() > 0.0f)
//            endAngleX -= 180.0f;
//        float barValue = m_graph->selectedSeries()->dataProxy()->itemAt(selectedBar.x(),
//                                                                        selectedBar.y())->value();
//        float endAngleY = barValue >= 0.0f ? 30.0f : -30.0f;
//        if (m_graph->valueAxis()->reversed())
//            endAngleY *= -1.0f;
//        //! [15]

//        m_animationCameraX.setEndValue(QVariant::fromValue(float(endAngleX)));
//        m_animationCameraY.setEndValue(QVariant::fromValue(endAngleY));
//        m_animationCameraZoom.setEndValue(QVariant::fromValue(250));
//        //! [14]
//        m_animationCameraTarget.setEndValue(QVariant::fromValue(endTarget));
//        //! [14]
//    } else {
//        // No selected bar, so return to the default view
//        m_animationCameraX.setEndValue(QVariant::fromValue(m_defaultAngleX));
//        m_animationCameraY.setEndValue(QVariant::fromValue(m_defaultAngleY));
//        m_animationCameraZoom.setEndValue(QVariant::fromValue(m_defaultZoom));
//        m_animationCameraTarget.setEndValue(QVariant::fromValue(m_defaultTarget));
//    }

//    m_animationCameraX.start();
//    m_animationCameraY.start();
//    m_animationCameraZoom.start();
//    m_animationCameraTarget.start();
//}
////! [11]

void MainWindow::func_GUIdefault() //GUIフォームの初期化
{
    //QSlider *rotationSliderX = new QSlider(Qt::Horizontal, widget);
    ui->rotationSliderX->setTickInterval(30);
    ui->rotationSliderX->setTickPosition(QSlider::TicksBelow);
    ui->rotationSliderX->setMinimum(-180);
    ui->rotationSliderX->setValue(0);
    ui->rotationSliderX->setMaximum(180);
    //QSlider *rotationSliderY = new QSlider(Qt::Horizontal, widget);
    ui->rotationSliderY->setTickInterval(15);
    ui->rotationSliderY->setTickPosition(QSlider::TicksAbove);
    ui->rotationSliderY->setMinimum(-90);
    ui->rotationSliderY->setValue(0);
    ui->rotationSliderY->setMaximum(90);
}

