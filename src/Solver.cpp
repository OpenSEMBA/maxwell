#include "Solver.h"

#include <fstream>
#include <iostream>
#include <algorithm>

using namespace mfem;


namespace Maxwell {

Solver::Solver(const Options& opts, const Mesh& mesh) 
{

    checkOptionsAreValid(opts, mesh);

    mesh_ = mfem::Mesh(mesh, true);
    opts_ = opts;

    //fes_ = buildFiniteElementSpace();

    Kx_ = buildDerivativeOperators(*"X");
    Kx_ = buildDerivativeOperators(*"Y");

    buildMassMatrix();
}

void Solver::checkOptionsAreValid(const Options& opts, const Mesh& mesh) 
{
    if (mesh.Dimension() != 2) {
        throw std::exception("Incorrect Dimension for mesh");
    }
    if ((opts.order < 0) ||
        (opts.t_final < 0) ||
        (opts.dt < 0) ||
        (opts.vis_steps < 1) ||
        (opts.precision < 1)) {
        throw std::exception("Incorrect parameters in Options");
    }

}

//std::unique_ptr<mfem::FiniteElementSpace> Solver::buildFiniteElementSpace() const
//{
//    DG_FECollection fec(opts_.order, mesh_.Dimension(), BasisType::GaussLobatto);
//    return std::make_unique<FiniteElementSpace>(&mesh_, &fec);
//}

void Solver::buildMassMatrix()
{
    MInv_ = std::make_unique<BilinearForm>(fes_.get());
    MInv_->AddDomainIntegrator(new InverseIntegrator(new MassIntegrator));
    MInv_->Assemble();
    MInv_->Finalize();
}

std::unique_ptr<mfem::BilinearForm> Solver::buildDerivativeOperators(const char direction)
{
    ConstantCoefficient zero(0.0), one(1.0); double auxValue; int indexValue;

    if (&direction == "X") {
        auxValue = 0.0; indexValue = 0;
    }
    else if(&direction == "Y") {
        auxValue = 1.0; indexValue = 1;
    }
    else {
        throw std::exception("Incorrect argument for direction in buildDerivativeOperators()");
    }

    Vector auxVec(2);  auxVec(0) = auxValue-0.0; auxVec(1) = auxValue-1.0;
    VectorConstantCoefficient vectorCoeff(auxVec);
    Vector n1Vec(2);  n1Vec(0) = 1.0; n1Vec(1) = 1.0;
    VectorConstantCoefficient n1(n1Vec);

    double alpha = -1.0, beta = 0.0; int skip_zeros = 0;

    std::unique_ptr<mfem::BilinearForm> kDir = std::make_unique<BilinearForm>(fes_.get());

    kDir->AddDomainIntegrator(new DerivativeIntegrator(one, indexValue));
    kDir->AddInteriorFaceIntegrator(
        new TransposeIntegrator(new DGTraceIntegrator(vectorCoeff, alpha, beta)));
    kDir->Assemble(skip_zeros);
    kDir->Finalize(skip_zeros);

    return kDir;

}

void Solver::setInitialFields(std::function<double(const mfem::Vector&)> f)
{
    GridFunction ez_(fes_.get()), hx_(fes_.get()), hy_(fes_.get());
    ez_.ProjectCoefficient(FunctionCoefficient(f));
    hx_.ProjectCoefficient(ConstantCoefficient(0.0));
    hy_.ProjectCoefficient(ConstantCoefficient(0.0));
}
//FunctionCoefficient u0(u0_function);
//GridFunction ez(fes), hx(fes), hy(fes);
//ez.ProjectCoefficient(u0);
//hx.ProjectCoefficient(zero);
//hy.ProjectCoefficient(zero);
//
//

void Solver::collectParaviewData()
{
    pd_ = NULL;
    pd_ = std::make_unique<ParaViewDataCollection>("Example9", &mesh_);
    pd_->SetPrefixPath("ParaView");
    pd_->RegisterField("ez", &ez_);
    pd_->RegisterField("hx", &hx_);
    pd_->RegisterField("hy", &hy_);
    pd_->SetLevelsOfDetail(opts_.order);
    pd_->SetDataFormat(VTKFormat::BINARY);
    opts_.order > 0 ? pd_->SetHighOrderOutput(true) : pd_->SetHighOrderOutput(false);
    pd_->SetCycle(0);
    pd_->SetTime(0.0);
    pd_->Save();
}


void Solver::run() 
{
    double time = 0.0;
    bool done = false;

    Vector aux(fes_->GetVSize());
    Vector ezNew(fes_->GetVSize());
    Vector hxNew(fes_->GetVSize());
    Vector hyNew(fes_->GetVSize());

    for (int cycle = 0; !done;)
    {

        // Update E.
        Kx_->Mult(hy_, aux);
        Ky_->AddMult(hx_, aux, -1.0);
        MInv_->Mult(aux, ezNew);
        ezNew *= -opts_.dt;
        ezNew.Add(1.0, ez_);


        // Update H.
        Kx_->Mult(ezNew, aux);
        MInv_->Mult(aux, hyNew);
        hyNew *= -opts_.dt;
        hyNew.Add(1.0, hy_);

        Ky_->Mult(ezNew, aux);
        MInv_->Mult(aux, hxNew);
        hxNew *= opts_.dt;
        hxNew.Add(1.0, hx_);

        ez_ = ezNew;
        hx_ = hxNew;
        hy_ = hyNew;

        time += opts_.dt;
        cycle++;

        done = (time >= opts_.t_final - 1e-8 * opts_.dt);

        if (done || cycle % opts_.vis_steps == 0) {
                pd_->SetCycle(cycle);
                pd_->SetTime(time);
                pd_->Save();
        }
    }
}
}
