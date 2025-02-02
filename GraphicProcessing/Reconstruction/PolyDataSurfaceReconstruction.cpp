/**********************************************************************

Copyright (c) Mr.Bin. All rights reserved.
For more information visit: http://blog.csdn.net/webzhuce

**********************************************************************/
#include <vtkSmartPointer.h>
#include <vtkSurfaceReconstructionFilter.h>
#include <vtkProgrammableSource.h>
#include <vtkContourFilter.h>
#include <vtkReverseSense.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkPolyData.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>
#include <vtkPolydataReader.h>
#include <vtkVertexGlyphFilter.h>

//�����ļ���../data/fran_cut.vtk
int main(int argc, char *argv[])
{
	vtkSmartPointer<vtkPolyDataReader> reader = 
		vtkSmartPointer<vtkPolyDataReader>::New();
	reader->SetFileName("F:/fran_cut.vtk");
	reader->Update();

	vtkSmartPointer<vtkPolyData> points = 
		vtkSmartPointer<vtkPolyData>::New();
	points->SetPoints(reader->GetOutput()->GetPoints());

	vtkSmartPointer<vtkSurfaceReconstructionFilter> surf = 
		vtkSmartPointer<vtkSurfaceReconstructionFilter>::New();
	surf->SetInputData(points);
	surf->SetNeighborhoodSize(20);
	surf->SetSampleSpacing(0.005);
	surf->Update();

	vtkSmartPointer<vtkContourFilter> contour = 
		vtkSmartPointer<vtkContourFilter>::New();
	contour->SetInputConnection(surf->GetOutputPort());
	contour->SetValue(0, 0.0);

	double leftViewport[4] = {0.0, 0.0, 0.5, 1.0};
	double rightViewport[4] = {0.5, 0.0, 1.0, 1.0};

	vtkSmartPointer<vtkVertexGlyphFilter> vertexGlyphFilter =
		vtkSmartPointer<vtkVertexGlyphFilter>::New();
	vertexGlyphFilter->AddInputData(points);

	vtkSmartPointer<vtkPolyDataMapper> vertexMapper = 
		vtkSmartPointer<vtkPolyDataMapper>::New();
	vertexMapper->SetInputConnection(vertexGlyphFilter->GetOutputPort());
	vertexMapper->ScalarVisibilityOff();

	vtkSmartPointer<vtkActor> vertexActor = 
		vtkSmartPointer<vtkActor>::New();
	vertexActor->SetMapper(vertexMapper);
	vertexActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

	vtkSmartPointer<vtkRenderer> vertexRenderer = 
		vtkSmartPointer<vtkRenderer>::New();
	vertexRenderer->AddActor(vertexActor);
	vertexRenderer->SetViewport(leftViewport);
	vertexRenderer->SetBackground(1.0, 1.0, 1.0);

	vtkSmartPointer<vtkPolyDataMapper> surfMapper = 
		vtkSmartPointer<vtkPolyDataMapper>::New();
	surfMapper->SetInputConnection(contour->GetOutputPort());
	surfMapper->ScalarVisibilityOff();

	vtkSmartPointer<vtkActor> surfActor = 
		vtkSmartPointer<vtkActor>::New();
	surfActor->SetMapper(surfMapper);
	surfActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
	//surfActor->GetProperty()->SetRepresentationToWireframe();

	vtkSmartPointer<vtkRenderer> surfRenderer = 
		vtkSmartPointer<vtkRenderer>::New();
	surfRenderer->AddActor(surfActor);
	surfRenderer->SetViewport(rightViewport);
	surfRenderer->SetBackground(1.0, 1.0, 1.0);

	vtkSmartPointer<vtkRenderWindow> renWin = 
		vtkSmartPointer<vtkRenderWindow>::New();
	renWin->AddRenderer(surfRenderer);
	renWin->AddRenderer(vertexRenderer);
	renWin->SetSize(640, 320);
	renWin->Render();
	renWin->SetWindowName("PolyDataSurfaceReconstruction");

	vtkSmartPointer<vtkRenderWindowInteractor> iren = 
		vtkSmartPointer<vtkRenderWindowInteractor>::New();
	iren->SetRenderWindow(renWin);

	renWin->Render();
	iren->Start();

	return EXIT_SUCCESS;
}