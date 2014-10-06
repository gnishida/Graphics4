#include "Projector.h"
#include "PPC.h"
#include "FrameBuffer.h"
#include "TMesh.h"

/**
 * Construct a project.
 *
 * @param ppc			the camera to capture the scene
 * @param fb0			the frame buffer for an object which will be projected
 * @param fb1			the frame buffer for a background to check the Z buffer
 */
Projector::Projector(PPC* ppc, FrameBuffer* fb0, FrameBuffer* fb1) {
	this->ppc = ppc;
	this->fb0 = fb0;
	this->fb1 = fb1;
}

void Projector::AddForegroundMesh(TMesh* mesh) {
	this->foremeshes.push_back(mesh);
}

void Projector::AddBackgroundMesh(TMesh* mesh) {
	this->backmeshes.push_back(mesh);
}

/**
 * Capture the scene so that the frame buffer is ready for projection.
 */
void Projector::Capture() {
	fb0->Set(WHITE);
	fb0->SetZB(0.0f);
	for (int i = 0; i < foremeshes.size(); i++) {
		foremeshes[i]->Render(fb0, ppc);
	}

	fb1->Set(WHITE);
	fb1->SetZB(0.0f);
	for (int i = 0; i < backmeshes.size(); i++) {
		backmeshes[i]->Render(fb1, ppc);
	}
}

void Projector::TranslateLR(float step) {
	ppc->TranslateLR(step);

	fb1->Set(WHITE);
	fb1->SetZB(0.0f);
	for (int i = 0; i < backmeshes.size(); i++) {
		backmeshes[i]->Render(fb1, ppc);
	}
}

void Projector::TranslateUD(float step) {
	ppc->TranslateUD(step);

	fb1->Set(WHITE);
	fb1->SetZB(0.0f);
	for (int i = 0; i < backmeshes.size(); i++) {
		backmeshes[i]->Render(fb1, ppc);
	}
}

void Projector::TranslateFB(float step) {
	ppc->TranslateFB(step);

	fb1->Set(WHITE);
	fb1->SetZB(0.0f);
	for (int i = 0; i < backmeshes.size(); i++) {
		backmeshes[i]->Render(fb1, ppc);
	}
}

void Projector::Pan(float angle) {
	ppc->Pan(angle);

	fb1->Set(WHITE);
	fb1->SetZB(0.0f);
	for (int i = 0; i < backmeshes.size(); i++) {
		backmeshes[i]->Render(fb1, ppc);
	}
}

void Projector::Tilt(float angle) {
	ppc->Tilt(angle);

	fb1->Set(WHITE);
	fb1->SetZB(0.0f);
	for (int i = 0; i < backmeshes.size(); i++) {
		backmeshes[i]->Render(fb1, ppc);
	}
}

void Projector::Roll(float angle) {
	ppc->Roll(angle);

	fb1->Set(WHITE);
	fb1->SetZB(0.0f);
	for (int i = 0; i < backmeshes.size(); i++) {
		backmeshes[i]->Render(fb1, ppc);
	}
}
