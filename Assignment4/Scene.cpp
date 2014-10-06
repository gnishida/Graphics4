#include "Scene.h"
#include "V3.h"
#include "Box.h"
#include "Triangle.h"
#include "Quad.h"
#include "Sphere.h"
#include "Light.h"
#include "FrameBuffer.h"
#include "Projector.h"
#include <time.h>
#include <float.h>
#include <iostream>
#include <fstream>

using namespace std;

#define HIGH_RES		1

Scene *scene;// = new Scene();
std::vector<Light*>* Scene::lights = new std::vector<Light*>();

Scene::Scene() {
	mipmap_mode = true;

	//lights.push_back(new Light(V3(30.0f, 0.0f, 30.0f), Light::TYPE_POINT_LIGHT, 0.4f, 0.6f, 40.0f));
	lights->push_back(new Light(V3(-30.0f, 0.0f, -30.0f), Light::TYPE_DIRECTIONAL_LIGHT, 0.4f, 0.6f, 40.0f));
	//lights.push_back(new Light(V3(-30.0f, 0.0f, 0.0f), Light::TYPE_DIRECTIONAL_LIGHT, 0.4f, 0.6f, 40.0f));

	// create user interface
	gui = new GUI();
	gui->show();

	// create SW framebuffer
	int u0 = 20;
	int v0 = 50;
	int sci = 2;
	int w = sci*240;//640;
	int h = sci*180;//360;
	win = new MainWindow(u0, v0, w, h);
	win->label("SW Framebuffer");
	win->show();
  
	// position UI window
	gui->uiw->position(win->frame->w+u0 + 2*20, v0);

	// create a cameras
	PPC* ppc = new PPC(60.0f, win->frame->w, win->frame->h);
	//ppc->LookAt(V3(0.0f, 0.0f, 0.0f), V3(-0.8333f, 0.0f, -1.0f), V3(0.0f, 1.0f, 0.0f), 200.0f);
	ppc->LookAt(V3(0.0f, 0.0f, 0.0f), V3(-1.0f, -1.0f, 0.0f), V3(-1.0f, 1.0f, 0.0f), 150.0f);
	ppcs.push_back(ppc);

	currentPPC = ppcs[0];

	//Save();
}

/**
 * This function is called when "Demo" button is clicked.
 */
void Scene::Demo() {
	win->frame->SetZB(0.0f);
	win->frame->Set(WHITE);

	// create a camera for the projector
	PPC ppc(60.0f, win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	ppc.LookAt(V3(0.0f, 0.0f, 0.0f), V3(-1.0f, -1.0f, 0.0f), V3(-1.0f, 1.0f, 0.0f), 150.0f);

	// create a frame buffer for the camera which is used for the projector
	FrameBuffer fb0(win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	fb0.Set(WHITE);
	fb0.SetZB(0.0f);
	TMesh mesh;
	mesh.Load("geometry/teapot57K.bin");
	mesh.Translate(mesh.GetCentroid() * -1.0f);

	// create a scene
	FrameBuffer fb1(win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	fb1.Set(WHITE);
	fb1.SetZB(0.0f);
	TMesh mesh2;
	mesh2.Load("geometry/auditorium.bin");
	mesh2.Translate(mesh2.GetCentroid() * -1.0f);
	mesh2.RotateAbout(V3(1.0f, 0.0f, 0.0f), -90.0f);
	AABB aabb;
	mesh2.ComputeAABB(aabb);	
	mesh2.Scale(V3(0, 0, 0), aabb.Size() * 50.0f);
	meshes.push_back(&mesh2);

	// create a projector
	Projector projector(&ppc, &fb0, &fb1);
	projector.AddForegroundMesh(&mesh);
	projector.AddBackgroundMesh(&mesh2);

	// create a camera
	PPC ppc2(60.0f, win->frame->w, win->frame->h);
	ppc2.LookAt(V3(0.0f, 0.0f, 0.0f), V3(-1.0f, -1.0f, 0.0f), V3(-1.0f, 1.0f, 0.0f), 150.0f);
	currentPPC = &ppc2;

	// apply the projector to the scene
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i]->SetProjector(&projector);
	}

	projector.Capture();

	for (int i = 0; i < 30; i++) {
		Render();
		Fl::wait();
	}

	for (int i = 30; i < 270; i++) {
		currentPPC->RotateAbout(V3(0.0f, 1.0f, 0.0f), -1.5f, V3(-150.0f, 0.0f, 23.0f));
		Render();
		Fl::wait();
	}

	for (int i = 270; i < 300; i++) {
		Render();
		Fl::wait();
	}

	// add a teapot to the scene
	meshes.push_back(&mesh);

	// create a camera for the shadow mapping
	PPC ppc3(100.0f, win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	ppc3.LookAt(V3(0.0f, 0.0f, 0.0f), V3(-1.0f, -1.0f, 0.0f), V3(-1.0f, 1.0f, 0.0f), 250.0f);

	// create a frame buffer for the shadow mapping
	FrameBuffer fb2(win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);

	// render a scene by a reference camera for the shadow mapping
	fb2.Set(WHITE);
	fb2.SetZB(0.0f);
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i]->Render(&fb2, &ppc3);
	}

	for (int i = 300; i < 330; i++) {
		RenderShadowMapping(&fb2, &ppc3);
		Fl::wait();
	}

	for (int i = 330; i < 570; i++) {
		currentPPC->RotateAbout(V3(0.0f, 1.0f, 0.0f), 1.5f, V3(0.0f, 0.0f, 0.0f));
		RenderShadowMapping(&fb2, &ppc3);
		Fl::wait();
	}

	for (int i = 570; i < 600; i++) {
		RenderShadowMapping(&fb2, &ppc3);
		Fl::wait();
	}

	meshes.erase(meshes.begin() + (meshes.size() - 1));
	meshes.erase(meshes.begin() + (meshes.size() - 1));
}

/**
 * This function is called when "Save" button is clicked.
 * The scene is animated with total 600 frames, which are corresponding to 20 seconds of animation,
 * and all the frames are stored in "captured" folder.
 */
void Scene::Save() {
	char filename[256];

	win->frame->SetZB(0.0f);
	win->frame->Set(WHITE);

	// create a camera for the projector
	PPC ppc(60.0f, win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	ppc.LookAt(V3(0.0f, 0.0f, 0.0f), V3(-1.0f, -1.0f, 0.0f), V3(-1.0f, 1.0f, 0.0f), 150.0f);

	// create a frame buffer for the camera which is used for the projector
	FrameBuffer fb0(win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	fb0.Set(WHITE);
	fb0.SetZB(0.0f);
	TMesh mesh;
	mesh.Load("geometry/teapot57K.bin");
	mesh.Translate(mesh.GetCentroid() * -1.0f);

	// create a scene
	FrameBuffer fb1(win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	fb1.Set(WHITE);
	fb1.SetZB(0.0f);
	TMesh mesh2;
	mesh2.Load("geometry/auditorium.bin");
	mesh2.Translate(mesh2.GetCentroid() * -1.0f);
	mesh2.RotateAbout(V3(1.0f, 0.0f, 0.0f), -90.0f);
	AABB aabb;
	mesh2.ComputeAABB(aabb);	
	mesh2.Scale(V3(0, 0, 0), aabb.Size() * 50.0f);
	meshes.push_back(&mesh2);

	// create a projector
	Projector projector(&ppc, &fb0, &fb1);
	projector.AddForegroundMesh(&mesh);
	projector.AddBackgroundMesh(&mesh2);

	// create a camera
	PPC ppc2(60.0f, win->frame->w, win->frame->h);
	ppc2.LookAt(V3(0.0f, 0.0f, 0.0f), V3(-1.0f, -1.0f, 0.0f), V3(-1.0f, 1.0f, 0.0f), 150.0f);
	currentPPC = &ppc2;

	// apply the projector to the scene
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i]->SetProjector(&projector);
	}

	projector.Capture();

	for (int i = 0; i < 30; i++) {
		Render();
		sprintf(filename, "captured\\scene%03d.tif", i);
		win->frame->Save(filename);
		Fl::wait();
	}

	for (int i = 30; i < 270; i++) {
		currentPPC->RotateAbout(V3(0.0f, 1.0f, 0.0f), -1.5f, V3(-150.0f, 0.0f, 23.0f));
		Render();
		sprintf(filename, "captured\\scene%03d.tif", i);
		win->frame->Save(filename);
		Fl::wait();
	}

	for (int i = 270; i < 300; i++) {
		Render();
		sprintf(filename, "captured\\scene%03d.tif", i);
		win->frame->Save(filename);
		Fl::wait();
	}

	// add a teapot to the scene
	meshes.push_back(&mesh);

	// create a camera for the shadow mapping
	PPC ppc3(100.0f, win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	ppc3.LookAt(V3(0.0f, 0.0f, 0.0f), V3(-1.0f, -1.0f, 0.0f), V3(-1.0f, 1.0f, 0.0f), 250.0f);

	// create a frame buffer for the shadow mapping
	FrameBuffer fb2(win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);

	// render a scene by a reference camera for the shadow mapping
	fb2.Set(WHITE);
	fb2.SetZB(0.0f);
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i]->Render(&fb2, &ppc3);
	}

	for (int i = 300; i < 330; i++) {
		RenderShadowMapping(&fb2, &ppc3);
		sprintf(filename, "captured\\scene%03d.tif", i);
		win->frame->Save(filename);
		Fl::wait();
	}

	for (int i = 330; i < 570; i++) {
		currentPPC->RotateAbout(V3(0.0f, 1.0f, 0.0f), 1.5f, V3(0.0f, 0.0f, 0.0f));
		RenderShadowMapping(&fb2, &ppc3);
		sprintf(filename, "captured\\scene%03d.tif", i);
		win->frame->Save(filename);
		Fl::wait();
	}

	for (int i = 570; i < 600; i++) {
		RenderShadowMapping(&fb2, &ppc3);
		sprintf(filename, "captured\\scene%03d.tif", i);
		win->frame->Save(filename);
		Fl::wait();
	}

	meshes.erase(meshes.begin() + (meshes.size() - 1));
	meshes.erase(meshes.begin() + (meshes.size() - 1));
}

/**
 * Projector Demo
 */
void Scene::ProjectorDemo() {
	// create a camera for the projector
	PPC ppc(60.0f, win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	ppc.LookAt(V3(0.0f, 0.0f, 0.0f), V3(-1.0f, -1.0f, 0.0f), V3(-1.0f, 1.0f, 0.0f), 150.0f);

	// create a frame buffer for the camera which is used for the projector
	FrameBuffer fb0(win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	fb0.Set(WHITE);
	fb0.SetZB(0.0f);
	TMesh mesh;
	mesh.Load("geometry/teapot57K.bin");
	mesh.Translate(mesh.GetCentroid() * -1.0f);

	// create a scene
	FrameBuffer fb1(win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	fb1.Set(WHITE);
	fb1.SetZB(0.0f);
	TMesh mesh2;
	mesh2.Load("geometry/auditorium.bin");
	mesh2.Translate(mesh2.GetCentroid() * -1.0f);
	mesh2.RotateAbout(V3(1.0f, 0.0f, 0.0f), -90.0f);
	AABB aabb;
	mesh2.ComputeAABB(aabb);	
	mesh2.Scale(V3(0, 0, 0), aabb.Size() * 50.0f);
	meshes.push_back(&mesh2);

	// create a projector
	Projector projector(&ppc, &fb0, &fb1);
	projector.AddForegroundMesh(&mesh);
	projector.AddBackgroundMesh(&mesh2);

	// capture the scene beforehand
	projector.Capture();

	// create a camera
	PPC ppc2(60.0f, win->frame->w, win->frame->h);
	ppc2.LookAt(V3(0.0f, 0.0f, 0.0f), V3(-1.0f, -1.0f, 0.0f), V3(-1.0f, 1.0f, 0.0f), 150.0f);
	currentPPC = &ppc2;

	// apply the projector to the scene
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i]->SetProjector(&projector);
	}

	for (int i = 0; i < 60; i++) {
		projector.TranslateLR(0.3f);
		//projector.ppc->TranslateLR(-0.3f);
		//projector.Capture();
		Render();
		Fl::wait();
	}

	for (int i = 60; i < 180; i++) {
		projector.TranslateLR(0.3f);
		//projector.ppc->TranslateLR(-0.3f);
		//projector.Capture();
		currentPPC->RotateAbout(V3(0.0f, 1.0f, 0.0f), -0.75f, V3(-150.0f, 0.0f, -10.0f));	
		Render();
		Fl::wait();
	}

	// remove the projector from the scene
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i]->SetProjector(NULL);
	}

	// remove the mesh
	meshes.erase(meshes.begin() + (meshes.size() - 1));

	currentPPC = ppcs[0];
}

/**
 * Save the project demo to tiff files.
 */
void Scene::SaveProjector() {
	char filename[256];

	// create a camera for the projector
	PPC ppc(60.0f, win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	//ppc.LookAt(V3(0.0f, 0.0f, 0.0f), V3(-1.0f, 0.0f, 0.0f), V3(0.0f, 1.0f, 0.0f), 200.0f);
	ppc.LookAt(V3(0.0f, 0.0f, 0.0f), V3(-1.0f, -1.0f, 0.0f), V3(-1.0f, 1.0f, 0.0f), 150.0f);

	// create a frame buffer for the camera which is used for the projector
	FrameBuffer fb0(win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	fb0.Set(WHITE);
	fb0.SetZB(0.0f);
	TMesh mesh;
	mesh.Load("geometry/teapot57K.bin");
	mesh.Translate(mesh.GetCentroid() * -1.0f);

	// create a scene
	FrameBuffer fb1(win->frame->w * HIGH_RES, win->frame->h * HIGH_RES);
	fb1.Set(WHITE);
	fb1.SetZB(0.0f);
	TMesh mesh2;
	mesh2.Load("geometry/auditorium.bin");
	mesh2.Translate(mesh2.GetCentroid() * -1.0f);
	mesh2.RotateAbout(V3(1.0f, 0.0f, 0.0f), -90.0f);
	AABB aabb;
	mesh2.ComputeAABB(aabb);	
	mesh2.Scale(V3(0, 0, 0), aabb.Size() * 50.0f);
	meshes.push_back(&mesh2);

	// create a projector
	Projector projector(&ppc, &fb0, &fb1);
	projector.AddForegroundMesh(&mesh);
	projector.AddBackgroundMesh(&mesh2);

	// capture the scene beforehand
	projector.Capture();

	// create a camera
	PPC ppc2(60.0f, win->frame->w, win->frame->h);
	ppc2.LookAt(V3(0.0f, 0.0f, 0.0f), V3(-1.0f, -1.0f, 0.0f), V3(-1.0f, 1.0f, 0.0f), 150.0f);
	currentPPC = &ppc2;

	// apply the projector to the scene
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i]->SetProjector(&projector);
	}

	for (int i = 0; i < 60; i++) {
		projector.TranslateLR(0.3f);
		Render();
		sprintf(filename, "captured\\scene%03d.tif", i);
		win->frame->Save(filename);
		Fl::wait();
	}

	for (int i = 60; i < 180; i++) {
		projector.TranslateLR(0.3f);
		currentPPC->RotateAbout(V3(0.0f, 1.0f, 0.0f), -0.75f, V3(-150.0f, 0.0f, -10.0f));	
		Render();
		sprintf(filename, "captured\\scene%03d.tif", i);
		win->frame->Save(filename);
		Fl::wait();
	}

	// remove the projector from the scene
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i]->SetProjector(NULL);
	}

	// remove the mesh
	meshes.erase(meshes.begin() + (meshes.size() - 1));

	currentPPC = ppcs[0];
}

/**
 * Render all the models.
 */
void Scene::Render() {
	win->frame->SetZB(0.0f);
	win->frame->Set(WHITE);

	for (int i = 0; i < meshes.size(); i++) {
		meshes[i]->Render(win->frame, currentPPC);
	}

	win->redraw();
}

void Scene::Render4Way(TMesh* object, Projector *projector) {
	/*
	win->frame->SetZB(0.0f);
	win->frame->Set(WHITE);

	// What the audience would see without the effect
	FrameBuffer fb(win->frame->w, win->frame->h);
	fb.SetZB(0.0f);
	fb.Set(WHITE);
	object->SetProjector(NULL);
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i]->Render(&fb, currentPPC);
	}
	for (int v = 0; v < fb.h / 2; v++) {
		for (int u = 0; u < fb.w / 2; u++) {
			V3 c1, c2, c3, c4;
			c1.SetColor(fb.pix[v * 2 * fb.w + u * 2]);
			c2.SetColor(fb.pix[v * 2 * fb.w + u * 2 + 1]);
			c3.SetColor(fb.pix[(v * 2 + 1) * fb.w + u * 2]);
			c4.SetColor(fb.pix[(v * 2 + 1) * fb.w + u * 2 + 1]);

			win->frame->pix[(v + fb.h / 2) * win->frame->w + u] = ((c1 + c2 + c3 + c4) * 0.25f).GetColor();
		}
	}

	// What the audience would see with the effect
	fb.SetZB(0.0f);
	fb.Set(WHITE);
	object->SetProjector(projector);
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i]->Render(&fb, currentPPC);
	}
	for (int v = 0; v < fb.h / 2; v++) {
		for (int u = 0; u < fb.w / 2; u++) {
			V3 c1, c2, c3, c4;
			c1.SetColor(fb.pix[v * 2 * fb.w + u * 2]);
			c2.SetColor(fb.pix[v * 2 * fb.w + u * 2 + 1]);
			c3.SetColor(fb.pix[(v * 2 + 1) * fb.w + u * 2]);
			c4.SetColor(fb.pix[(v * 2 + 1) * fb.w + u * 2 + 1]);

			win->frame->pix[(v + fb.h / 2) * win->frame->w + fb.w / 2 + u] = ((c1 + c2 + c3 + c4) * 0.25f).GetColor();
		}
	}

	// What the camera captures
	int scale = projector->fb->w / fb.w * 2;
	for (int v = 0; v < fb.h / 2; v++) {
		for (int u = 0; u < fb.w / 2; u++) {
			win->frame->pix[v * win->frame->w + u] = projector->fb->pix[v * scale * projector->fb->w + u * scale];
		}
	}

	// What the projector projects
	fb.SetZB(0.0f);
	fb.Set(WHITE);
	object->Render(&fb, projector->ppc);

	win->redraw();
	*/
}

void Scene::RenderProjectiveTextureMapping(FrameBuffer* fb, PPC* ppc) {
	win->frame->SetZB(0.0f);
	win->frame->Set(WHITE);

	// projective texture mapping
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i]->RenderProjectiveTextureMapping(fb, ppc, win->frame, currentPPC);
	}

	win->redraw();
}

void Scene::RenderShadowMapping(FrameBuffer* fb, PPC* ppc) {
	win->frame->SetZB(0.0f);
	win->frame->Set(WHITE);

	for (int i = 0; i < meshes.size(); i++) {
		meshes[i]->RenderShadowMapping(fb, ppc, win->frame, currentPPC);
	}

	win->redraw();
}

TIFFImage* Scene::CreateCubeMap(int size) {
	PPC ppc(90.0f, size, size);

	// render the back scene
	FrameBuffer fb1(size, size);
	fb1.SetZB(0.0f);
	fb1.Set(BLACK);
	for (int i= 0; i < meshes.size(); i++) {
		meshes[i]->Render(&fb1, &ppc);
	}

	// render the left scene
	FrameBuffer fb2(size, size);
	ppc.Pan(90.0f);
	fb2.SetZB(0.0f);
	fb2.Set(BLACK);
	for (int i= 0; i < meshes.size(); i++) {
		meshes[i]->Render(&fb2, &ppc);
	}

	// render the front scene
	FrameBuffer fb3(size, size);
	ppc.Pan(90.0f);
	fb3.SetZB(0.0f);
	fb3.Set(BLACK);
	for (int i= 0; i < meshes.size(); i++) {
		meshes[i]->Render(&fb3, &ppc);
	}

	// render the right scene
	FrameBuffer fb4(size, size);
	ppc.Pan(90.0f);
	fb4.SetZB(0.0f);
	fb4.Set(BLACK);
	for (int i= 0; i < meshes.size(); i++) {
		meshes[i]->Render(&fb4, &ppc);
	}

	// render the top scene
	FrameBuffer fb5(size, size);
	ppc.Pan(90.0f);
	ppc.Tilt(90.0f);
	fb5.SetZB(0.0f);
	fb5.Set(BLACK);
	for (int i= 0; i < meshes.size(); i++) {
		meshes[i]->Render(&fb5, &ppc);
	}

	// render the bottom scene
	FrameBuffer fb6(size, size);
	ppc.Tilt(-180.0f);
	fb6.SetZB(0.0f);
	fb6.Set(BLACK);
	for (int i= 0; i < meshes.size(); i++) {
		meshes[i]->Render(&fb6, &ppc);
	}

	int w = size * 3;
	int h = size * 4;
	TIFFImage* result = new TIFFImage(w, h);
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			result->pix[x + size + y * w] = fb1.pix[x + (size - y - 1) * size];
			result->pix[x + (y + size * 2) * w] = fb2.pix[(size - x - 1) + y * size];
			result->pix[x + size + (y + size * 2) * w] = fb3.pix[(size - x - 1) + y * size];
			result->pix[x + size * 2 + (y + size * 2) * w] = fb4.pix[(size - x - 1) + y * size];
			result->pix[x + size + (y + size * 3) * w] = fb5.pix[(size - x - 1) + y * size];
			result->pix[x + size + (y + size) * w] = fb6.pix[x + (size - y - 1) * size];
		}
	}

	return result;
}

V3 Scene::RayTrace(PPC* ppc, const V3 &p, const V3 &dir, float &dist) {
	float dist_min = std::numeric_limits<float>::max();
	V3 c_min;

	for (int i = 0; i < meshes.size(); i++) {
		float d;
		V3 c;
		if (!meshes[i]->RayTrace(ppc, p, dir, c, d)) continue;
		if (d < dist_min) {
			dist_min = d;
			c_min = c;
		}
	}

	dist = dist_min;

	return c_min;
}