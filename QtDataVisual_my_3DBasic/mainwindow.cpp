#include "mainwindow.h"
#include "ui_mainwindow.h"

//参考：
//・3Dグラフ表示　座標ファイルを読み込んで散布図(scatterとして表示)　（マウス操作：ズームあり、視点移動なし）
//https://github.com/fengfanchen/Qt/tree/master/3D%20move%20rotation
//
//・参考：Qt付属のExampleコード　customplot(CSVファイルデータ入力でグラフ表示), bars　（マウス操作：ズームあり、視点移動あり）

#include <QtDataVisualization/QScatter3DSeries>
#include <QDebug>
#include <QThread>
#include <QMessageBox>
#include <QHBoxLayout>

//-start- Qt_Official_sample:custominput(CSVファイルデータ入力でグラフ表示) , bars(視点移動)
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

using namespace QtDataVisualization;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_xRotation(0.0f)
    , m_yRotation(0.0f)
{
    ui->setupUi(this);
    this->setWindowTitle("3D obj-file . QtDataVisualization");
    func_GUIdefault(); //GUIフォーム初期化 rotation設定など

    //m_graph = new Q3DScatter();
    m_graph = new Q3DSurface();

    m_graph->axisX()->setRange(-10, 10);
    m_graph->axisY()->setRange(-10, 10);
    m_graph->axisZ()->setRange(-10, 10);

//    QScatter3DSeries *series = new QScatter3DSeries;
//    series->setItemLabelFormat(QStringLiteral("@xLabel, @yLabel, @zLabel"));
//    series->setMesh(QAbstract3DSeries::MeshCube);
//    series->setItemSize(0.15f);
//    m_graph->addSeries(series); // m_graph->seriesList().at(0) の作成

    QWidget *container = QWidget::createWindowContainer(m_graph);

    if (!m_graph->hasContext()) {
        QMessageBox msgBox;
        msgBox.setText("Couldn't initialize the OpenGL context.");
        msgBox.exec();
        return;
    }

//    QScatter3DSeries *item = new QScatter3DSeries(); //表示の時、必須。
//    item->setMesh(QAbstract3DSeries::MeshUserDefined); //表示の時、必須。
//    addData(); //自作関数：csvファイルからデータを読み込んで追加

    //设置到控件上　コントロールに設定→ Widgetに登録、画面で表示されるようになる
    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget(container, 1);
    ui->widget->setLayout(hLayout);

    //-start- 視点移動ができるように設定 Qt-Official-sample bars
    changePresetCamera();
    // Set up property animations for zooming to the selected bar
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
    //-end- 視点移動ができるように設定 Qt-Official-sample bars


}

MainWindow::~MainWindow()
{
    qDebug() << "Widget::~Widget()";
    delete m_graph;
    delete ui;
}


//-start-  視点移動 Qt-official-sample bar より流用
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
//-end-  視点移動 Qt-official-sample bar より流用
