#include "mfem.hpp"
#include <fstream>
#include <iostream>
#include <../../maxwell/src/maxwell/Types.h>
#include <../../maxwell/src/maxwell/BilinearIntegrators.h>

#include <Eigen/Dense>
#include <maxwell/Model.h>

using namespace mfem;

struct FluxCoefficient {
	double alpha;
	double beta;
};

std::unique_ptr<FiniteElementSpace> buildFiniteElementSpace(const int order)
{
	Mesh mesh = Mesh::MakeCartesian1D(1);
	std::unique_ptr<FiniteElementCollection> fec = std::make_unique<DG_FECollection>(order, 1, BasisType::GaussLobatto);
	return std::make_unique<FiniteElementSpace>(&mesh, fec.get());
}

Eigen::MatrixXd convertMFEMDenseToEigen(DenseMatrix* mat)
{
	auto res = Eigen::MatrixXd(mat->Width(), mat->Height());
	for (int i = 0; i < mat->Width(); i++) {
		for (int j = 0; j < mat->Height(); j++) {
			res(i, j) = mat->Elem(i, j);
		}
	}
	return res;
}

Eigen::MatrixXd buildMassMatrixEigen(std::unique_ptr<FiniteElementSpace>& fes)
{
	ConstantCoefficient one(1.0);
	BilinearForm res(fes.get());
	res.AddDomainIntegrator(new MassIntegrator(one));
	res.Assemble();
	res.Finalize();

	return convertMFEMDenseToEigen(res.SpMat().ToDenseMatrix());
}

Eigen::MatrixXd buildInverseMassMatrixEigen(std::unique_ptr<FiniteElementSpace>& fes)
{
	ConstantCoefficient one(1.0);
	BilinearForm res(fes.get());
	res.AddDomainIntegrator(new InverseIntegrator(new MassIntegrator(one)));
	res.Assemble();
	res.Finalize();

	return convertMFEMDenseToEigen(res.SpMat().ToDenseMatrix());
}

Eigen::MatrixXd buildStiffnessMatrixEigen(std::unique_ptr<FiniteElementSpace>& fes)
{
	ConstantCoefficient one(1.0);
	BilinearForm res(fes.get());
	res.AddDomainIntegrator(new DerivativeIntegrator(one, 0));
	res.Assemble();
	res.Finalize();

	return convertMFEMDenseToEigen(res.SpMat().ToDenseMatrix());
}

Eigen::MatrixXd	buildNormalPECFluxOperator1D(std::unique_ptr<FiniteElementSpace>& fes, std::vector<maxwell::Direction> dirVec)
{
	std::vector<maxwell::Direction> dirs = dirVec;
	maxwell::AttributeToBoundary attBdr{ {1,maxwell::BdrCond::PEC},{2,maxwell::BdrCond::PEC} };

	auto res = std::make_unique<BilinearForm>(fes.get());
	{
		FluxCoefficient c = FluxCoefficient{ 0.0, -0.5 };
		res->AddInteriorFaceIntegrator(new maxwell::MaxwellDGTraceJumpIntegrator(dirs, c.beta));
		//res->AddInteriorFaceIntegrator(new DGTraceIntegrator(*(new VectorConstantCoefficient(Vector(1.0))), 0.0, 0.5));
	}

	std::vector<Array<int>> bdrMarkers;
	bdrMarkers.resize(fes.get()->GetMesh()->bdr_attributes.Max());
	for (auto const& kv : attBdr) {
		Array<int> bdrMarker(fes.get()->GetMesh()->bdr_attributes.Max());
		bdrMarker = 0;
		bdrMarker[(int)kv.first - 1] = 1;

		bdrMarkers[(int)kv.first - 1] = bdrMarker;
		FluxCoefficient c = FluxCoefficient{ 0.0, -2.0 };
		res->AddBdrFaceIntegrator(new maxwell::MaxwellDGTraceJumpIntegrator(dirs, c.beta), bdrMarkers[kv.first - 1]);
		//res->AddBdrFaceIntegrator(new DGTraceIntegrator(*(new VectorConstantCoefficient(Vector(1.0))), 0.0, 2.0), bdrMarkers[kv.first - 1]);

		res->Assemble();
		res->Finalize();

		return convertMFEMDenseToEigen(res->SpMat().ToDenseMatrix());
	}
}