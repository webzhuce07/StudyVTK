/**********************************************************************

Copyright (c) Mr.Bin. All rights reserved.
For more information visit: http://blog.csdn.net/webzhuce

**********************************************************************/
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkStructuredPoints.h>
#include <vtkStructuredPointsReader.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolumeProperty.h>
#include <vtkSmartVolumeMapper.h>

//测试：../data/mummy.128.vtk
int main(int argc, char *argv[])
{
	vtkSmartPointer<vtkStructuredPointsReader> reader =
		vtkSmartPointer<vtkStructuredPointsReader>::New();
	reader->SetFileName("E:\\TestData\\mummy.128.vtk");
	reader->Update();

	vtkSmartPointer<vtkSmartVolumeMapper> volumeMapper =
		vtkSmartPointer<vtkSmartVolumeMapper>::New();
	volumeMapper->SetInputData(reader->GetOutput());
	//设置光线采样距离
	//volumeMapper->SetSampleDistance(volumeMapper->GetSampleDistance()*4);
	//设置图像采样步长
	//volumeMapper->SetAutoAdjustSampleDistances(0);
	//volumeMapper->SetImageSampleDistance(4);

	vtkSmartPointer<vtkVolumeProperty> volumeProperty = 
		vtkSmartPointer<vtkVolumeProperty>::New();
	volumeProperty->SetInterpolationTypeToLinear();
	//volumeProperty->ShadeOn();  //打开或者关闭阴影测试
	volumeProperty->SetAmbient(0.4);
	volumeProperty->SetDiffuse(0.6);
	volumeProperty->SetSpecular(0.2);

	vtkSmartPointer<vtkPiecewiseFunction> compositeOpacity = 
		vtkSmartPointer<vtkPiecewiseFunction>::New();
	compositeOpacity->AddPoint(70,   0.00);
	compositeOpacity->AddPoint(90,   0.40);
	compositeOpacity->AddPoint(180,  0.60);
	volumeProperty->SetScalarOpacity(compositeOpacity); //设置不透明度传输函数

	//测试隐藏部分数据,对比不同的设置
	//compositeOpacity->AddPoint(120,  0.00);
	//compositeOpacity->AddPoint(180,  0.60);
	//volumeProperty->SetScalarOpacity(compositeOpacity);

	vtkSmartPointer<vtkPiecewiseFunction> volumeGradientOpacity =
		vtkSmartPointer<vtkPiecewiseFunction>::New();
	volumeGradientOpacity->AddPoint(10,  0.0);
	volumeGradientOpacity->AddPoint(90,  0.5);
	volumeGradientOpacity->AddPoint(100, 1.0);
	//volumeProperty->SetGradientOpacity(volumeGradientOpacity);//设置梯度不透明度效果对比

	vtkSmartPointer<vtkColorTransferFunction> color = 
		vtkSmartPointer<vtkColorTransferFunction>::New();
	color->AddRGBPoint(0.000,  0.00, 0.00, 0.00);
	color->AddRGBPoint(64.00,  1.00, 0.52, 0.30);
	color->AddRGBPoint(190.0,  1.00, 1.00, 1.00);
	color->AddRGBPoint(220.0,  0.20, 0.20, 0.20);
	volumeProperty->SetColor(color);

	vtkSmartPointer<vtkVolume> volume = 
		vtkSmartPointer<vtkVolume>::New();
	volume->SetMapper(volumeMapper);
	volume->SetProperty(volumeProperty);

	vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
	ren->SetBackground(1.0, 1.0, 1.0);
	ren->AddVolume( volume ); 

	vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
	renWin->AddRenderer(ren);
	renWin->SetSize(640, 480);
	renWin->Render();
	renWin->SetWindowName("VolumeRendering");

	vtkSmartPointer<vtkRenderWindowInteractor> iren = 
		vtkSmartPointer<vtkRenderWindowInteractor>::New();
	iren->SetRenderWindow(renWin);
	ren->ResetCamera();

	renWin->Render();
	iren->Start();

	return EXIT_SUCCESS;
}