#include "Types.h"

namespace maxwell {

using namespace mfem;
using namespace mfemExtension;
using FiniteElementOperator = BilinearForm;

struct MaxwellEvolOptions {
	FluxType fluxType{ FluxType::Upwind };
};

FluxCoefficient interiorFluxCoefficient()
{
	return FluxCoefficient{ 1.0, 0.0 };
}

FluxCoefficient interiorPenaltyFluxCoefficient(const MaxwellEvolOptions& opts)
{
	switch (opts.fluxType) {
	case FluxType::Centered:
		return FluxCoefficient{ 0.0, 0.0 };
	case FluxType::Upwind:
		return FluxCoefficient{ 0.0, 0.5 };
	default:
		throw std::exception("No defined FluxType.");
	}
}

FluxCoefficient boundaryFluxCoefficient(const FieldType& f, const BdrCond& bdrC)
{
	switch (bdrC) {
	case BdrCond::PEC:
		switch (f) {
		case FieldType::E:
			return FluxCoefficient{ 0.0, 0.0 };
		case FieldType::H:
			return FluxCoefficient{ 2.0, 0.0 };
		}
	case BdrCond::PMC:
		switch (f) {
		case FieldType::E:
			return FluxCoefficient{ 2.0, 0.0 };
		case FieldType::H:
			return FluxCoefficient{ 0.0, 0.0 };
		}
	case BdrCond::SMA:
		switch (f) {
		case FieldType::E:
			return FluxCoefficient{ 1.0, 0.0 };
		case FieldType::H:
			return FluxCoefficient{ 1.0, 0.0 };
		}
	default:
		throw std::exception("No defined BdrCond.");
	}
}

FluxCoefficient boundaryPenaltyFluxCoefficient(const FieldType& f, const BdrCond& bdrC, const MaxwellEvolOptions& opts)
{
	switch (opts.fluxType) {
	case FluxType::Centered:
		return FluxCoefficient{ 0.0, 0.0 };
	case FluxType::Upwind:
		switch (bdrC) {
		case BdrCond::PEC:
			switch (f) {
			case FieldType::E:
				return FluxCoefficient{ 0.0, 0.0 };
			case FieldType::H:
				return FluxCoefficient{ 0.0, 0.0 };
			}
		case BdrCond::PMC:
			switch (f) {
			case FieldType::E:
				return FluxCoefficient{ 0.0, 0.0 };
			case FieldType::H:
				return FluxCoefficient{ 0.0, 0.0 };
			}
		case BdrCond::SMA:
			switch (f) {
			case FieldType::E:
				return FluxCoefficient{ -1.0, 0.0 };
			case FieldType::H:
				return FluxCoefficient{ -1.0, 0.0 };
			}
		default:
			throw std::exception("No defined BdrCond.");
		}
	default:
		throw std::exception("No defined FluxType.");
	}
}

FieldType altField(const FieldType& f)
{
	switch (f) {
	case E:
		return H;
	case H:
		return E;
	}
	throw std::runtime_error("Invalid field type for altField.");

}

FiniteElementOperator buildByMult(
	const BilinearForm& op1,
	const BilinearForm& op2,
	FiniteElementSpace& fes)
{
	auto aux = mfem::Mult(op1.SpMat(), op2.SpMat());
	auto res = BilinearForm(&fes);
	res.Assemble();
	res.Finalize();
	res.SpMat().Swap(*aux);

	return res;
}

Vector buildNVector(const Direction& d, const FiniteElementSpace& fes)
{
	const auto dim{ fes.GetMesh()->Dimension() };
	assert(d < dim);

	Vector r(dim);
	r = 0.0;
	r[d] = 1.0;
	return r;
}

FiniteElementOperator buildInverseMassMatrix(const FieldType& f, const Model& model, FiniteElementSpace& fes)
{
	Vector aux{ model.buildPiecewiseArgVector(f) };
	PWConstCoefficient PWCoeff(aux);

	auto MInv = BilinearForm(&fes);
	MInv.AddDomainIntegrator(new InverseIntegrator(new MassIntegrator(PWCoeff)));

	MInv.Assemble();
	MInv.Finalize();
	return MInv;
}



FiniteElementOperator buildDerivativeOperator(const Direction& d, FiniteElementSpace& fes)
{
	auto res = BilinearForm(&fes);

	if (d >= fes.GetMesh()->Dimension()) {
		res.Assemble();
		res.Finalize();
		return res;
	}

	ConstantCoefficient coeff(1.0);
	res.AddDomainIntegrator(
		new TransposeIntegrator(
			new DerivativeIntegrator(coeff, d)
		)
	);

	res.Assemble();
	res.Finalize();
	return res;
}

FiniteElementOperator buildFluxOperator(const FieldType& f, const Direction& d, bool usePenaltyCoefficients, Model& model, FiniteElementSpace& fes, const MaxwellEvolOptions& opts)
{
	auto res = BilinearForm(&fes);
	if (d >= fes.GetMesh()->Dimension()) {
		res.Assemble();
		res.Finalize();
		return res;
	}

	Vector aux = buildNVector(d, fes);
	VectorConstantCoefficient n(aux);
	{
		FluxCoefficient c;
		if (usePenaltyCoefficients) {
			c = interiorPenaltyFluxCoefficient(opts);
		}
		else {
			c = interiorFluxCoefficient();
		}
		res.AddInteriorFaceIntegrator(new MaxwellDGTraceIntegrator(n, c.alpha, c.beta));
	}

	for (auto& kv : model.getBoundaryToMarker()) {
		FluxCoefficient c;
		if (usePenaltyCoefficients) {
			c = boundaryPenaltyFluxCoefficient(f, kv.first, opts);
		}
		else {
			c = boundaryFluxCoefficient(f, kv.first);
		}
		res.AddBdrFaceIntegrator(
			new MaxwellDGTraceIntegrator(n, c.alpha, c.beta), kv.second
		);
	}

	res.Assemble();
	res.Finalize();
	return res;
}


}