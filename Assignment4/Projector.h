#pragma once

#include <vector>

class PPC;
class FrameBuffer;
class TMesh;

class Projector {
public:
	PPC* ppc;

	/** The frame buffer for an object which will be projected */
	FrameBuffer* fb0;

	/** The frame buffer for a background to check the Z buffer */
	FrameBuffer* fb1;

	std::vector<TMesh*> foremeshes;
	std::vector<TMesh*> backmeshes;

public:
	Projector(PPC* ppc, FrameBuffer* fb0, FrameBuffer* fb1);
	void AddForegroundMesh(TMesh* mesh);
	void AddBackgroundMesh(TMesh* mesh);
	void Capture();
	void TranslateLR(float step);
	void TranslateUD(float step);
	void TranslateFB(float step);
	void Pan(float angle);
	void Tilt(float angle);
	void Roll(float angle);
};

