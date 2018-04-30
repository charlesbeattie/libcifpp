// Lib for working with structures as contained in mmCIF and PDB files

#pragma once

#include <set>
#include <tuple>
#include <vector>
#include <map>

#include "cif++/AtomType.h"
#include "cif++/Cif++.h"

namespace libcif
{

// --------------------------------------------------------------------
// The chemical composition of the structure in an mmCIF file is 
// defined in the class composition. A compositon consists of
// entities. Each Entity can be either a polymer, a non-polymer
// a macrolide or a water molecule.
// Entities themselves are made up of compounds. And compounds
// contain CompoundAtom records for each atom.

class Composition;
class Entity;
class Compound;
struct CompoundAtom;

// --------------------------------------------------------------------
// struct containing information about an atom in a chemical compound
// This information comes from the CCP4 monomer library. 

struct CompoundAtom
{
	std::string	id;
	AtomType	typeSymbol;
	std::string	typeEnergy;
	float		partialCharge;
};

// --------------------------------------------------------------------
// struct containing information about the bonds
// This information comes from the CCP4 monomer library. 

enum CompoundBondType { singleBond, doubleBond, tripleBond, delocalizedBond };

struct CompoundBond
{
	std::string			atomID[2];
	CompoundBondType	type;
	float				distance;
	float				esd;
};

// --------------------------------------------------------------------
// struct containing information about the bond-angles
// This information comes from the CCP4 monomer library. 

struct CompoundAngle
{
	std::string			atomID[3];
	float				angle;
	float				esd;
};

// --------------------------------------------------------------------
// struct containing information about the bond-angles
// This information comes from the CCP4 monomer library. 

struct CompoundPlane
{
	std::string					id;
	std::vector<std::string>	atomID;
	float						esd;
};

// --------------------------------------------------------------------
// struct containing information about a chiral centre
// This information comes from the CCP4 monomer library. 

enum ChiralVolumeSign { negativ, positiv, both };

struct ChiralCentre
{
	std::string			id;
	std::string			atomIDCentre;
	std::string			atomID[3];
	ChiralVolumeSign	volumeSign;
};

// --------------------------------------------------------------------
// a class that contains information about a chemical compound.
// This information is derived from the ccp4 monomer library by default.
// To create compounds, you'd best use the factory method.

class Compound
{
  public:

	Compound(const boost::filesystem::path& file, const std::string& id, const std::string& name,
		const std::string& group);

	// factory method, create a Compound based on the three letter code
	// (for amino acids) or the one-letter code (for bases) or the
	// code as it is known in the CCP4 monomer library.

	static const Compound* create(const std::string& id);

	// this second factory method can create a Compound even if it is not
	// recorded in the library. It will take the values from the CCP4 lib
	// unless the value passed to this function is not empty.
	static const Compound* create(const std::string& id, const std::string& name,
		const std::string& type, const std::string& formula);
	
	// add an additional path to the monomer library.
	static void addMonomerLibraryPath(const std::string& dir);

	// accessors
	std::string id() const							{ return mId; }
	std::string	name() const						{ return mName; }
	std::string	type() const;
	std::string group() const						{ return mGroup; }
	std::vector<CompoundAtom> atoms() const			{ return mAtoms; }
	std::vector<CompoundBond> bonds() const			{ return mBonds; }
	std::vector<CompoundAngle> angles() const		{ return mAngles; }
	std::vector<ChiralCentre> chiralCentres() const	{ return mChiralCentres; }
	std::vector<CompoundPlane> planes() const		{ return mPlanes; }
	
	CompoundAtom getAtomById(const std::string& atomId) const;
	
	bool atomsBonded(const std::string& atomId_1, const std::string& atomId_2) const;
	float atomBondValue(const std::string& atomId_1, const std::string& atomId_2) const;
	float bondAngle(const std::string& atomId_1, const std::string& atomId_2, const std::string& atomId_3) const;
	float chiralVolume(const std::string& centreID) const;

	std::string formula() const;
	float formulaWeight() const;
	int charge() const;
	bool isWater() const;
	bool isSugar() const;

	std::vector<std::string>	isomers() const;
	bool isIsomerOf(const Compound& c) const;
	std::vector<std::tuple<std::string,std::string>> mapToIsomer(const Compound& c) const;

  private:

	~Compound();

	cif::File					mCF;

	std::string					mId;
	std::string					mName;
	std::string					mGroup;
	std::vector<CompoundAtom>	mAtoms;
	std::vector<CompoundBond>	mBonds;
	std::vector<CompoundAngle>	mAngles;
	std::vector<ChiralCentre>	mChiralCentres;
	std::vector<CompoundPlane>	mPlanes;
};

// --------------------------------------------------------------------
// Factory class for Compound objects

extern const std::map<std::string,char> kAAMap, kBaseMap;

class CompoundFactory
{
  public:
	
	static CompoundFactory& instance();

	void pushDictionary(const std::string& inDictFile);
	void popDictionary();

	bool isKnownPeptide(const std::string& res_name) const;
	bool isKnownBase(const std::string& res_name) const;

	std::string unalias(const std::string& res_name) const;

	const Compound* get(std::string id);
	const Compound* create(std::string id);

  private:

	CompoundFactory();
	~CompoundFactory();
	
	CompoundFactory(const CompoundFactory&) = delete;
	CompoundFactory& operator=(const CompoundFactory&) = delete;
	
	static CompoundFactory* sInstance;

	struct CompoundFactoryImpl* mImpl;
};

}
