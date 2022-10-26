#include "MaxwellEvolution2D.h"

namespace maxwell {

using namespace mfem;
using namespace mfemExtension;

MaxwellEvolution2D::MaxwellEvolution2D(
	FiniteElementSpace& fes, Model& model, MaxwellEvolOptions& options) :
	TimeDependentOperator(numberOfFieldComponents * numberOfMaxDimensions * fes.GetNDofs()),
	fes_{ fes },
	model_{ model },
	opts_{ options }
{
	for (auto f : { E, H }) {
		MP_[f] = buildByMult(*buildInverseMassMatrix(f, model_, fes_), *buildPenaltyOperator2D(f, std::vector<Direction>{}, model_, fes_, opts_), fes_);
		for (auto d : { X, Y, Z }) {
			MS_[f][d] = buildByMult(*buildInverseMassMatrix(f, model_, fes_), *buildDerivativeOperator(d, fes_), fes_);
			for (auto d2 : { X,Y,Z }) {
				for (auto f2 : { E, H }) {
					MFN_[f][f2][d] = buildByMult(*buildInverseMassMatrix(f, model_, fes_), *buildFluxOperator2D(f2, std::vector<Direction>{d}, model_, fes_, opts_), fes_);
					MFNN_[f][f2][d][d2] = buildByMult(*buildInverseMassMatrix(f, model_, fes_), *buildFluxOperator2D(f2, std::vector<Direction>{d, d2}, model_, fes_, opts_), fes_);
				}
			}
		}
	}
}

void MaxwellEvolution2D::Mult(const Vector& in, Vector& out) const
{
	std::array<Vector, 3> eOld, hOld;
	std::array<GridFunction, 3> eNew, hNew;
	for (int d = X; d <= Z; d++) {
		eOld[d].SetDataAndSize(in.GetData() + d * fes_.GetNDofs(), fes_.GetNDofs());
		hOld[d].SetDataAndSize(in.GetData() + (d + 3) * fes_.GetNDofs(), fes_.GetNDofs());
		eNew[d].MakeRef(&fes_, &out[d * fes_.GetNDofs()]);
		hNew[d].MakeRef(&fes_, &out[(d + 3) * fes_.GetNDofs()]);
	}

	// Flux term for Hx. LIFT*(Fscale.*FluxHx) = LIFT*(Fscale.*(ny.*dEz + alpha*(nx.*dHx.*nx+ny.*dHy.*nx-dHx)))/2.0;
	MFNN_[H][H][X][X]->   Mult(hOld[X], hNew[X]);
	MFNN_[H][H][Y][X]->AddMult(hOld[Y], hNew[X]);
	MP_[H]		     ->AddMult(hOld[X], hNew[X], -1.0);
	MFN_[H][E][Y]    ->AddMult(eOld[Z], hNew[X]);
	hNew[X].operator/=(2.0);

	//Mass term for Hx.
	MS_[H][Y]        ->AddMult(eOld[Z], hNew[X], -1.0);

	// Flux term for Hy. LIFT*(Fscale.*FluxHy) = LIFT*(Fscale.*(-nx.*dEz + alpha*(nx.*dHx.*ny+ny.*dHy.*ny-dHy)))/2.0;
	MFNN_[H][H][X][Y]->   Mult(hOld[X], hNew[Y]);
	MFNN_[H][H][Y][Y]->AddMult(hOld[Y], hNew[Y]);
	MP_[H]           ->AddMult(hOld[Y], hNew[Y], -1.0);
	MFN_[H][E][X]    ->AddMult(eOld[Z], hNew[Y], -1.0);
	hNew[Y].operator/=(2.0);				 

	// Mass term for Hy.
	MS_[H][X]        ->AddMult(eOld[Z], hNew[Y]);

	// Flux term for Ez. LIFT*(Fscale.*FluxEz) = LIFT*(Fscale.*(-nx.*dHy + ny.*dHx - alpha*dEz))/2.0;

	MFN_[H][H][Y]->	  Mult(hOld[X], eNew[Z]);
	MFN_[H][H][X]->AddMult(hOld[Y], eNew[Z], -1.0);
	MP_[E]       ->AddMult(eOld[Z], eNew[Z], -1.0);
	eOld[Z].operator/=(2.0);

	// Mass term for Ez.
	MS_[E][X]	 ->AddMult(hOld[Y], eNew[Z]);
	MS_[E][Y]    ->AddMult(hOld[X], eNew[Z], -1.0);


	//for (int x = X; x <= Z; x++) {
	//	int y = (x + 1) % 3;
	//	int z = (x + 2) % 3;

	//	 dtE_x = MS_y * H_z - MF_y * {H_z} - MP_E * [E_z] +
	//	        -MS_z * H_y + MF_z * {H_y} + MP_E * [E_y]
	//	 Update E.
	//	MS_[E][z]->Mult   (hOld[y], eNew[x]);
	//	MF_[E][z]->AddMult(hOld[y], eNew[x], -1.0);
	//	MP_[E][z]->AddMult(eOld[y], eNew[x], -1.0);
	//	MS_[E][y]->AddMult(hOld[z], eNew[x], -1.0);
	//	MF_[E][y]->AddMult(hOld[z], eNew[x],  1.0);
	//	MP_[E][y]->AddMult(eOld[z], eNew[x],  1.0); 

	//	// Update H.
	//	MS_[H][y]->Mult   (eOld[z], hNew[x]);
	//	MF_[H][y]->AddMult(eOld[z], hNew[x], -1.0);
	//	MP_[H][y]->AddMult(hOld[z], hNew[x], -1.0);
	//	MS_[H][z]->AddMult(eOld[y], hNew[x], -1.0);
	//	MF_[H][z]->AddMult(eOld[y], hNew[x],  1.0);
	//	MP_[H][z]->AddMult(hOld[y], hNew[x],  1.0);
	//}

}

}
