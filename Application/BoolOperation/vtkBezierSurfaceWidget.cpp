/****************************************************************************
**
** Copyright (C) VCreate Logic Private Limited, Bangalore
**
** Use of this file is limited according to the terms specified by
** VCreate Logic Private Limited, Bangalore. Details of those terms
** are listed in licence.txt included as part of the distribution package
** of this file. This file may not be distributed without including the
** licence.txt file.
**
** Contact info@vcreatelogic.com if any conditions of this licensing are
** not clear to you.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/**
Authors:
    Prashanth N Udupa (prashanth@vcreatelogic.com)
    Brian Gee Chacko (brian.chacko@vcreatelogic.com)
*/

#include "vtkBezierSurfaceWidget.h"

#include "vtkOutputWindow.h"
#include "vtkRenderer.h"
#include "vtkBezierSurfaceSource.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkCallbackCommand.h"
#include "vtkPropPicker.h"
#include "vtkSTLWriter.h"

typedef unsigned int uint;

vtkStandardNewMacro(vtkBezierSurfaceWidget);

struct HandleInfo
{
    vtkSphereSource* Source;
    vtkPolyDataMapper* Mapper;
    vtkActor* Actor;

    // Control point indexes
    int xCPIndex;
    int yCPIndex;

    HandleInfo()
    {
        this->Source = 0;
        this->Mapper = 0;

        this->xCPIndex = -1;
        this->yCPIndex = -1;
    }

    void Init()
    {
        if(this->Source)
            return;

        this->Source = vtkSphereSource::New();
        this->Mapper = vtkPolyDataMapper::New();
        this->Actor = vtkActor::New();

        this->Source->SetThetaResolution(16);
        this->Source->SetPhiResolution(16);


        this->Mapper->SetInputConnection(this->Source->GetOutputPort());
        this->Actor->SetMapper(this->Mapper);
    }

    void SetRadius(double r)
    {
        if(this->Source)
            this->Source->SetRadius(r);
    }

    void SetPosition(double x, double y, double z)
    {
        if(this->Source)
            this->Source->SetCenter(x, y, z);
    }

    void SetPosition(double* pt) { SetPosition(pt[0], pt[1], pt[2]); }

    double* GetPosition()
    {
        return this->Source->GetCenter();
    }

    void GetPosition(double p[3])
    {
        double* pt = GetPosition();
        p[0] = pt[0];
        p[1] = pt[1];
        p[2] = pt[2];
    }

    void SetVisibility(int val)
    {
        if(this->Actor)
            this->Actor->SetVisibility(val);
    }

    void SetProperty(vtkProperty* prop)
    {
        if(this->Actor)
            this->Actor->SetProperty(prop);
    }

    void Finish()
    {
        this->Source->Delete();
        this->Mapper->Delete();
        this->Actor->Delete();
        delete this;
    }
};

vtkBezierSurfaceWidget::vtkBezierSurfaceWidget()
{
    this->Source = 0;
    this->Property = vtkProperty::New();
    this->Property->Register(this);

    this->Picker = vtkPropPicker::New();
    this->Picker->Register(this);

    this->CPGridMapper = 0;
    this->CPGridActor = 0;

    this->CurrHandleIndex = -1;

    this->EventCallbackCommand->SetCallback(vtkBezierSurfaceWidget::ProcessEvents);
}

vtkBezierSurfaceWidget::~vtkBezierSurfaceWidget()
{
    this->DestroyHandles();
    if(this->Property)
        this->Property->UnRegister(this);
    if(this->Source)
        this->Property->UnRegister(this);
    if(this->Picker)
        this->Picker->UnRegister(this);
    if(this->CPGridActor)
        this->CPGridActor->Delete();
}

void vtkBezierSurfaceWidget::PrintSelf(ostream& os, vtkIndent indent)
{
    vtk3DWidget::PrintSelf(os, indent);
}

void vtkBezierSurfaceWidget::SetSource(vtkBezierSurfaceSource* source)
{
    if(this->Source == source)
        return;

    if(this->Source)
    {
        this->Source->UnRegister(this);
        this->CPGridActor->Delete();
        this->CPGridMapper = 0;
        this->CPGridActor = 0;
        this->DestroyHandles();
    }

    this->Source = source;

    if(this->Source)
    {
        this->Source->Register(this);

        this->CPGridMapper = vtkPolyDataMapper::New();
        this->CPGridActor = vtkActor::New();
        this->CPGridMapper->SetInputData( this->Source->GetControlPointsOutput() );
        this->CPGridActor->SetMapper(this->CPGridMapper);
        this->CPGridMapper->Delete();
    }

    if(this->Interactor && this->Source)
        SetEnabled(1);
    else
        SetEnabled(0);
}

vtkBezierSurfaceSource* vtkBezierSurfaceWidget::GetSource()
{
    return this->Source;
}

void vtkBezierSurfaceWidget::SetProperty(vtkProperty* property)
{
    if(this->Property == property)
        return;

    if(this->Property)
    {
        this->Property->UnRegister(this);
        for(uint i=0; i<this->HandleInfoList.size(); i++)
            this->HandleInfoList[i]->SetProperty(0);
    }

    this->Property = property;

    if(this->Property)
    {
        this->Property->Register(this);
        for(uint i=0; i<this->HandleInfoList.size(); i++)
            this->HandleInfoList[i]->SetProperty(this->Property);
    }
}

vtkProperty* vtkBezierSurfaceWidget::GetProperty()
{
    return this->Property;
}

void vtkBezierSurfaceWidget::SetInteractor(vtkRenderWindowInteractor* iren)
{
    vtk3DWidget::SetInteractor(iren);
    if(this->Interactor && this->Source)
        SetEnabled(1);
    else
        SetEnabled(0);
}

void vtkBezierSurfaceWidget::SetProp3D(vtkProp3D*)
{
    vtkOutputWindow::GetInstance()->DisplayWarningText("SetProp3D() is disabled. Use SetSource() instead");
}

void vtkBezierSurfaceWidget::SetInput(vtkDataSet* dataSet)
{
    vtkBezierSurfaceSource* bss = reinterpret_cast<vtkBezierSurfaceSource*>(dataSet);
    if(!bss)
        vtkOutputWindow::GetInstance()->DisplayWarningText("SetInput() is disabled. Use SetSource() instead");
    else
        SetSource(bss);
}

void vtkBezierSurfaceWidget::SetEnabled(int enabled)
{
    if(this->Enabled == enabled)
        return;

    if(enabled)
    {
        if( !(this->Interactor && this->Source) )
            return;
    }

    if(enabled)
    {
        this->ConstructHandles();

        // Start the interactor callbacks
        vtkRenderWindowInteractor *i = this->Interactor;
        i->AddObserver(vtkCommand::MouseMoveEvent,
                   this->EventCallbackCommand,this->Priority);
        i->AddObserver(vtkCommand::LeftButtonPressEvent,
                   this->EventCallbackCommand, this->Priority);
        i->AddObserver(vtkCommand::LeftButtonReleaseEvent,
                   this->EventCallbackCommand, this->Priority);

        // Enable handle visibility
        for(uint i=0; i<this->HandleInfoList.size(); i++)
            this->HandleInfoList[i]->SetVisibility(1);
        if(this->CPGridActor)
        {
            vtkRenderer* ren = this->GetRenderer();
            if(ren)
                ren->AddActor(this->CPGridActor);
        }

        this->InvokeEvent(vtkCommand::EnableEvent, NULL);
    }

    if(!enabled)
    {
        // Disable handle visibility
        for(uint i=0; i<this->HandleInfoList.size(); i++)
            this->HandleInfoList[i]->SetVisibility(0);
        if(this->CPGridActor)
        {
            vtkRenderer* ren = this->GetRenderer();
            if(ren)
                ren->RemoveActor(this->CPGridActor);
        }

        this->InvokeEvent(vtkCommand::DisableEvent, NULL);
    }

    if(this->Interactor->GetRenderWindow())
        this->Interactor->GetRenderWindow()->Render();

    this->Enabled = enabled;

    if(enabled)
        this->SizeHandles();
}

void vtkBezierSurfaceWidget::PlaceWidget(double[6])
{

}

void vtkBezierSurfaceWidget::SetPlaceFactor(double)
{
    vtkOutputWindow::GetInstance()->DisplayWarningText("SetPlaceFactor() is disabled");
}

void vtkBezierSurfaceWidget::SetHandleSize(double)
{
    vtkOutputWindow::GetInstance()->DisplayWarningText("SetHandleSize() is disabled");
}

vtkRenderer* vtkBezierSurfaceWidget::GetRenderer()
{
    // Get the renderer
    vtkRenderer* ren = this->GetDefaultRenderer();
    if(!ren)
        ren = this->GetCurrentRenderer();
    if(!ren)
    {
        ren = this->Interactor->FindPokedRenderer(
                this->Interactor->GetLastEventPosition()[0],
                this->Interactor->GetLastEventPosition()[1]
            );
        this->SetCurrentRenderer(ren);
    }

    return ren;
}

void vtkBezierSurfaceWidget::DestroyHandles()
{
    vtkRenderer* ren = this->GetRenderer();

    // Remove old actors first
    for(uint i=0; i<this->HandleInfoList.size(); i++)
    {
        HandleInfo* info = this->HandleInfoList[i];
        if(ren)
            ren->RemoveActor(info->Actor);
        info->Finish();
    }
    this->HandleInfoList.clear();
}

void vtkBezierSurfaceWidget::SizeHandles()
{
    //double radius = this->vtk3DWidget::SizeHandles(1.5);
    double radius = 0.1;
    for(uint i=0; i<this->HandleInfoList.size(); i++)
    {
        HandleInfo* info = this->HandleInfoList[i];
        info->SetRadius(radius);
    }
}

void vtkBezierSurfaceWidget::ConstructHandles()
{
    vtkRenderer* ren = this->GetRenderer();

    if(!this->Source || !ren)
        return;

    // Construct as many mappers and actors as control points
    int* vec = this->Source->GetNumberOfControlPoints();
    int nrControlPts = vec[0] * vec[1];
    int index = 0;
    this->HandleInfoList.resize(nrControlPts, (HandleInfo*)0);
    for(int i=0; i<vec[0]; i++)
    {
        for(int j=0; j<vec[1]; j++)
        {
            double* pt = this->Source->GetControlPoint(i, j);

            HandleInfo* info = new HandleInfo;
            info->Init();
            info->SetPosition(pt);
            info->SetProperty(this->Property);
            info->SetVisibility(this->GetEnabled());

            // Store the control point indexes too to retrieve them later
            info->xCPIndex = i;
            info->yCPIndex = j;

            this->HandleInfoList[index++] = info;

            ren->AddActor(info->Actor);

            // Add the actor to pick list
            this->Picker->AddPickList(info->Actor);
        }
    }

    this->Picker->PickFromListOn();
    this->SizeHandles();
}

void vtkBezierSurfaceWidget::SelectHandle(int index)
{
    if(index < 0 || index >= (int)this->HandleInfoList.size())
        return;

    HandleInfo *info = this->HandleInfoList[index];

    // Cannot modify this->Property; because that will alter everything.
    vtkProperty *p = vtkProperty::New();
    p->DeepCopy(this->Property); // base the new property on the current one.
    p->SetColor(1.0, 0.0, 0.0);
    info->SetProperty(p);
    p->Delete();

    // Update the current index value from the HandleInfoList
    this->CurrHandleIndex = index;
}

void vtkBezierSurfaceWidget::UnSelectCurrentHandle()
{
    if(this->CurrHandleIndex < 0)
        return;

    HandleInfo *info = this->HandleInfoList[this->CurrHandleIndex];
    info->SetProperty(this->Property);
    this->CurrHandleIndex = -1;
}

void vtkBezierSurfaceWidget::ProcessEvents(vtkObject*, unsigned long event, void* clientdata, void*)
{
    vtkBezierSurfaceWidget* self = reinterpret_cast<vtkBezierSurfaceWidget *>(clientdata);

    switch(event)
    {
        case vtkCommand::LeftButtonPressEvent:
            self->OnLeftButtonDown();
            break;
        case vtkCommand::LeftButtonReleaseEvent:
            self->OnLeftButtonUp();
            break;
        case vtkCommand::MouseMoveEvent:
            self->OnMouseMove();
            break;
        default:
            break;
    }
}

void vtkBezierSurfaceWidget::OnLeftButtonDown()
{
    this->SizeHandles();

    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];

    vtkRenderer *ren = this->GetCurrentRenderer();

    if(!ren)
        return;

    // Pick from the renderer
    this->Picker->Pick(double(x), double(y), 0.0, ren);

    // Store the current pick position in an array
    this->Picker->GetPickPosition(this->LastPickPosition);

    vtkActor *pickedActor = this->Picker->GetActor();
    if(!pickedActor)
    {
        UnSelectCurrentHandle();
        return;
    }

    // Highlight the picked handle
    this->CurrHandleIndex = -1;
    for(uint i=0; i<this->HandleInfoList.size(); i++)
    {
        HandleInfo *info = this->HandleInfoList[i];
        if(pickedActor == info->Actor)
        {
            this->SelectHandle(i);
            break;
        }
    }

    // Handle Index not found
    if(this->CurrHandleIndex < 0)
        return;

    this->EventCallbackCommand->SetAbortFlag(1);
    this->StartInteraction();
    this->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
    this->Interactor->Render();
}


void vtkBezierSurfaceWidget::OnMouseMove()
{
    this->SizeHandles();

    // If no handle index was found
    if(this->CurrHandleIndex < 0)
        return;

    // Motion vector calculations
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];

    double focalPoint[4], pickPoint[4], prevPickPoint[4], currPos[3];
    this->ComputeWorldToDisplay(this->LastPickPosition[0], this->LastPickPosition[1],
                                this->LastPickPosition[2], focalPoint);
    this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),
                                double(this->Interactor->GetLastEventPosition()[1]),
                                focalPoint[2], prevPickPoint);
    this->ComputeDisplayToWorld(double(x), double(y), focalPoint[2], pickPoint);

    // Get the handle info of the current handle
    HandleInfo *info = this->HandleInfoList[this->CurrHandleIndex];
    double p[3];

    // Get the current position of the handle
    info->GetPosition(currPos);

    // Current handle centre + motion vector
    p[0] = currPos[0] + ( pickPoint[0] - prevPickPoint[0] );
    p[1] = currPos[1] + ( pickPoint[1] - prevPickPoint[1] );
    p[2] = currPos[2] + ( pickPoint[2] - prevPickPoint[2] );

    // Modify the handle position and the control point position
    info->SetPosition(p[0], p[1], p[2]);

    // Dont modify the control point yet, modify it upon mouse release

    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
    this->Interactor->Render();
}

void vtkBezierSurfaceWidget::OnLeftButtonUp()
{
    this->SizeHandles();

    // If no handle index was found
    if(this->CurrHandleIndex < 0)
        return;

    // Get the handle info of the current handle
    HandleInfo *info = this->HandleInfoList[this->CurrHandleIndex];
    double p[3];
    info->GetPosition(p);

    this->Source->SetControlPoint(info->xCPIndex, info->yCPIndex, p);

    this->UnSelectCurrentHandle();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->EndInteraction();
    this->InvokeEvent(vtkCommand::EndInteractionEvent, NULL);
    this->Interactor->Render();

	vtkNew<vtkSTLWriter> writer;
	writer->SetFileName("./surface.stl");
	writer->SetInputData(this->Source->GetOutput());
	writer->Update();
}

