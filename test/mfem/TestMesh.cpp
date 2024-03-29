#include "gtest/gtest.h"

#include <iostream>
#include <fstream>
#include <vector>

#include <mfem.hpp>

using namespace mfem;

double linearFunction(const Vector& pos)
{
	double normalizedPos;
	double leftBoundary = 0.0, rightBoundary = 1.0;
	double length = rightBoundary - leftBoundary;
	normalizedPos = (pos[0] - leftBoundary) / length;

	return 2 * normalizedPos;
}

class TestMesh : public ::testing::Test {
protected:
	void SetUp() override 
	{
		mesh_ = Mesh::MakeCartesian1D(1);
	}

	Mesh mesh_;

	static std::string getFilename(const std::string fn)
	{
		return "./testData/" + fn;
	}

	std::vector<int> mapQuadElementTopLeftVertex(const Mesh& mesh)
	{
		std::vector<int> res;
		for (int i = 0; i < mesh.GetNE(); i++) {
			Array<int> meshArrayElement;
			mesh.GetElementVertices(i, meshArrayElement);
			res.push_back(meshArrayElement[0]);
		}
		return res;
	}

	Mesh makeTwoAttributeCartesianMesh1D(const int& refTimes = 0)
	{
		Mesh res = Mesh::MakeCartesian1D(2);
		res.SetAttribute(0, 1);
		res.SetAttribute(1, 2);

		for (int i = 0; i < refTimes; i++) {
			res.UniformRefinement();
		}

		return res;
	}
};

TEST_F(TestMesh, TwoAttributeMesh)
{
	/*The purpose of this test is to check the makeTwoAttributeCartesianMesh1D(const int& refTimes)
	function.

	First, an integer is declared for the number of times we wish to refine the mesh, then a mesh is
	constructed with two elements, left and right hand sides, setting the following attributes.

	|------LHS------|------RHS------|

	|##ATTRIBUTE 1##|##ATTRIBUTE 2##|

	Once the mesh is refined, it is returned, then we compare if the expected number of elements is
	true for the actual elements in the mesh.

	Then, we consider how the mesh will perform its uniform refinement, and we declare that the
	LHS elements with Attribute one will be Even index elements (starting at 0), and the RHS
	elements with Attribute 2 will be Uneven index elements (starting at 1).*/

	const int refTimes = 3;
	Mesh mesh = makeTwoAttributeCartesianMesh1D(refTimes);

	EXPECT_EQ(pow(2, refTimes + 1), mesh.GetNE());
	for (int i = 0; i < mesh.GetNE(); i++) {
		if (i % 2 == 0) {
			EXPECT_EQ(1, mesh.GetAttribute(i));
		}
		else {
			EXPECT_EQ(2, mesh.GetAttribute(i));
		}
	}
}
TEST_F(TestMesh, MeshDimensions)
{
	/*This test ensures that the number of elements of any 2D Cartesian
	mesh is equal to the product of the horizontal and vertical segments

	Dimensional parameters are declared which are then used to create
	a mesh object, then the test comparison is made.*/


	int nx = 8; int ny = 8; bool generateEdges = true;
	Mesh mesh = Mesh::MakeCartesian2D(nx, ny, Element::QUADRILATERAL, generateEdges);

	EXPECT_EQ(nx * ny, mesh.GetNE());

}
TEST_F(TestMesh, DataValueOutsideNodesForOneElementMeshes)
{
	/* The purpose of this test is to ensure we can extract data from a GridFunction,
	even if the point we're trying to obtain it at is not necessarily a DoF or node.
	
	First, the basic process to declare and initialise a FiniteElementSpace is done,
	this means variables such as dimension, order, creating a mesh, a FEC and a finally,
	the FES.
	
	A GridFunction is then created and assigned the FES. A function is projected in the
	GridFunction, which is a linear function with a slope of 2.
	
	Lastly, an IntegrationPoint is constructed, which we will use to obtain the values
	from the GridFunction at any point we want. As the slope of the line is 2, we expect
	the values to be 2 times the xVal.*/
	
	Mesh mesh = Mesh::MakeCartesian1D(1);
	auto fecDG = new DG_FECollection(1, 1, BasisType::GaussLobatto);
	auto* fesDG = new FiniteElementSpace(&mesh, fecDG);

	GridFunction solution(fesDG);
	solution.ProjectCoefficient(FunctionCoefficient(linearFunction));
	IntegrationPoint integPoint;
	for (double xVal = 0.0; xVal <= 1; xVal = xVal + 0.1) {
		integPoint.Set(xVal, 0.0, 0.0, 0.0);
		double interpolatedPoint = solution.GetValue(0, integPoint);
		EXPECT_NEAR(xVal * 2, interpolatedPoint,1e-10);
	}
}
TEST_F(TestMesh, MeshElementVertices)
{
	/*This test was created to understand the process of mesh creation
	and assignation of vertex index to elements.

	First, dimensional variables are declared, which are then used
	to create a new mesh object.

	Then firstElementVerticesVector and lastElementVerticesVector are
	initialized and assigned values manually, with the values we expect
	these elements will have. We also retrieve the vertices for the first
	and last element of the mesh, and then store them inside vectors.

	Lastly, we compare that the vertices we retrieved from the mesh are
	equal to those we presumed at the start.*/

	int nx = 8; int ny = 8; bool generateEdges = true;
	Mesh mesh = Mesh::MakeCartesian2D(nx, ny, Element::QUADRILATERAL, generateEdges);

	std::vector<int> firstElementVerticesVector = { 0, 1, nx + 2, nx + 1 };
	std::vector<int> lastElementVerticesVector = { nx - 1, nx, nx * 2 + 1, nx * 2 };
	Array<int> meshArrayFirstElement;
	Array<int> meshArrayLastElement;

	mesh.GetElementVertices(0, meshArrayFirstElement);
	mesh.GetElementVertices(nx * ny - 1, meshArrayLastElement);

	std::vector<int> vectorFirstElement(meshArrayFirstElement.begin(), meshArrayFirstElement.end());
	std::vector<int> vectorLastElement(meshArrayLastElement.begin(), meshArrayLastElement.end());

	EXPECT_EQ(firstElementVerticesVector, vectorFirstElement);
	EXPECT_EQ(lastElementVerticesVector, vectorLastElement);

}
TEST_F(TestMesh, mapMeshElementAndVertex)
{

	/* This test was created with the aim to understand the mapping and ordering process
	of a mesh in a more visual way. It uses the mapQuadElementTopLeftVertex() function
	which, for a Quadrilateral Element, it extracts its top left vertex, which allows for a nigh
	full mapping of the mesh.

	First, dimensional variables are declared and a mesh is constructed.

	Then, the mapQuadElementTopLeftVertex extracts the top left vertex of each element and stores them
	in an integer vector.

	Lastly, we compare that the first mapped vertex is the first created vertex in the mesh 0,
	the top left vertex for the uppermost, rightmost element is equal to the last element's index - 1
	(due to how mesh mapping works), and the size of the mapped vertices vector is equal to the number
	of elements in the mesh - 1 (as it starts with index 0).*/

	int nx = 5; int ny = 5; bool generateEdges = true;
	Mesh mesh = Mesh::MakeCartesian2D(nx, ny, Element::QUADRILATERAL, generateEdges);

	std::vector<int> mapped = mapQuadElementTopLeftVertex(mesh);

	EXPECT_EQ(0, mapped[0]);
	EXPECT_EQ(nx - 1, mapped[mapped.size() - 1]);
	EXPECT_EQ(nx * ny - 1, mapped.size() - 1);

}

TEST_F(TestMesh, meshDataFileRead)
{

	ASSERT_NO_THROW(Mesh::LoadFromFile("./TestData/twotriang.mesh", 1, 0));

}


