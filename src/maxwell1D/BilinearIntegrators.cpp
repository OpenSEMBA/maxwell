#include "BilinearIntegrators.h"
namespace Maxwell1D {

class MaxwellDGTraceIntegrator : public BilinearFormIntegrator
{
public:
	//When undeclared, rho = 1.0;
	MaxwellDGTraceIntegrator(VectorCoefficient& u_, double a)
	{
		rho = NULL; u = &u_; alpha = a; beta = 0.5 * a; gamma = 0.0;
	}

	MaxwellDGTraceIntegrator(VectorCoefficient& u_, double a, double b)
	{
		rho = NULL; u = &u_; alpha = a; beta = b; gamma = 0.0;
	}

	MaxwellDGTraceIntegrator(VectorCoefficient& u_, double a, double b, double g)
	{
		rho = NULL; u = &u_; alpha = a; beta = b; gamma = g;
	}

	MaxwellDGTraceIntegrator(Coefficient& rho_, VectorCoefficient& u_,
		double a, double b)
	{
		rho = &rho_; u = &u_; alpha = a; beta = b;
	}

	MaxwellDGTraceIntegrator(Coefficient& rho_, VectorCoefficient& u_,
		double a, double b, double g)
	{
		rho = &rho_; u = &u_; alpha = a; beta = b; gamma = g;
	}

	using BilinearFormIntegrator::AssembleFaceMatrix;
	virtual void AssembleFaceMatrix(const FiniteElement& el1,
		const FiniteElement& el2,
		FaceElementTransformations& Trans,
		DenseMatrix& elmat);

	using BilinearFormIntegrator::AssemblePA;

	virtual void AssemblePAInteriorFaces(const FiniteElementSpace& fes);

	virtual void AssemblePABoundaryFaces(const FiniteElementSpace& fes);

	virtual void AddMultTransposePA(const Vector& x, Vector& y) const;

	virtual void AddMultPA(const Vector&, Vector&) const;

	virtual void AssembleEAInteriorFaces(const FiniteElementSpace& fes,
		Vector& ea_data_int,
		Vector& ea_data_ext,
		const bool add);

	virtual void AssembleEABoundaryFaces(const FiniteElementSpace& fes,
		Vector& ea_data_bdr,
		const bool add);

	static const IntegrationRule& GetRule(Geometry::Type geom, int order,
		FaceElementTransformations& T);

protected:
	Coefficient* rho;
	VectorCoefficient* u;
	double alpha, beta, gamma;
	// PA extension
	Vector pa_data;
	const DofToQuad* maps;             ///< Not owned
	const FaceGeometricFactors* geom;  ///< Not owned
	int dim, nf, nq, dofs1D, quad1D;

private:
	Vector shape1_, shape2_;
	void SetupPA(const FiniteElementSpace& fes, FaceType type);

};

void MaxwellDGTraceIntegrator::AssemblePAInteriorFaces(const FiniteElementSpace& fes)
{
	SetupPA(fes, FaceType::Interior);
}

void MaxwellDGTraceIntegrator::AssemblePABoundaryFaces(const FiniteElementSpace& fes)
{
	SetupPA(fes, FaceType::Boundary);
}

const IntegrationRule& MaxwellDGTraceIntegrator::GetRule(
	Geometry::Type geom, int order, FaceElementTransformations& T)
{
	int int_order = T.Elem1->OrderW() + 2 * order;
	return IntRules.Get(geom, int_order);
}

//void MaxwellDGTraceIntegrator::AddMultTransposePA(const Vector& x, Vector& y) const
//{
//	PADGTraceApplyTranspose(dim, dofs1D, quad1D, nf,
//		maps->B, maps->Bt,
//		pa_data, x, y);
//}

//Required functions that don't belong to MaxwellDGTraceIntegrator

}