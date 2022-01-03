/*-
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Copyright (c) 2020 NKI/AVL, Netherlands Cancer Institute
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <numeric>

#include "cif++/AtomType.hpp"
#include "cif++/Cif++.hpp"
#include "cif++/Compound.hpp"
#include "cif++/Point.hpp"

/*
	To modify a structure, you will have to use actions.
	
	The currently supported actions are:
	
//	- Move atom to new location
	- Remove atom
//	- Add new atom that was formerly missing
//	- Add alternate Residue
	- 
	
*/

namespace mmcif
{

class Atom;
class Residue;
class Monomer;
class Polymer;
class Structure;
class File;

// --------------------------------------------------------------------

class Atom
{
  public:
	Atom();
	Atom(struct AtomImpl *impl);
	Atom(const Atom &rhs);

	Atom(cif::Datablock &db, cif::Row &row);

	// a special constructor to create symmetry copies
	Atom(const Atom &rhs, const Point &symmmetry_location, const std::string &symmetry_operation);

	~Atom();

	explicit operator bool() const { return mImpl_ != nullptr; }

	// return a copy of this atom, with data copied instead of referenced
	Atom clone() const;

	Atom &operator=(const Atom &rhs);

	const std::string &id() const;
	AtomType type() const;

	Point location() const;
	void location(Point p);

	/// \brief Translate the position of this atom by \a t
	void translate(Point t);

	/// \brief Rotate the position of this atom by \a q
	void rotate(Quaternion q);

	// for direct access to underlying data, be careful!
	const cif::Row getRow() const;
	const cif::Row getRowAniso() const;

	// Atom symmetryCopy(const Point& d, const clipper::RTop_orth& rt);
	bool isSymmetryCopy() const;
	std::string symmetry() const;
	// const clipper::RTop_orth& symop() const;

	const Compound &comp() const;
	bool isWater() const;
	int charge() const;

	float uIso() const;
	bool getAnisoU(float anisou[6]) const;
	float occupancy() const;

	template <typename T>
	T property(const std::string_view name) const;

	void property(const std::string_view name, const std::string &value);

	template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
	void property(const std::string_view name, const T &value)
	{
		property(name, std::to_string(value));
	}

	// specifications
	const std::string& labelAtomID() const		{ return mAtomID; }
	const std::string& labelCompID() const		{ return mCompID; }
	const std::string& labelAsymID() const		{ return mAsymID; }
	std::string labelEntityID() const;
	int labelSeqID() const						{ return mSeqID; }
	const std::string& labelAltID() const		{ return mAltID; }
	bool isAlternate() const					{ return not mAltID.empty(); }

	std::string authAtomID() const;
	std::string authCompID() const;
	std::string authAsymID() const;
	std::string authSeqID() const;
	std::string pdbxAuthInsCode() const;
	std::string pdbxAuthAltID() const;

	std::string labelID() const; // label_comp_id + '_' + label_asym_id + '_' + label_seq_id
	std::string pdbID() const;   // auth_comp_id + '_' + auth_asym_id + '_' + auth_seq_id + pdbx_PDB_ins_code

	bool operator==(const Atom &rhs) const;

	// // get clipper format Atom
	// clipper::Atom toClipper() const;

	// Radius calculation based on integrating the density until perc of electrons is found
	void calculateRadius(float resHigh, float resLow, float perc);
	float radius() const;

	// access data in compound for this atom

	// convenience routine
	bool isBackBone() const
	{
		auto atomID = labelAtomID();
		return atomID == "N" or atomID == "O" or atomID == "C" or atomID == "CA";
	}

	void swap(Atom &b)
	{
		std::swap(mImpl_, b.mImpl_);
		std::swap(mAtomID, b.mAtomID);
		std::swap(mCompID, b.mCompID);
		std::swap(mAsymID, b.mAsymID);
		std::swap(mSeqID, b.mSeqID);
		std::swap(mAltID, b.mAltID);
	}

	int compare(const Atom &b) const;

	bool operator<(const Atom &rhs) const
	{
		return compare(rhs) < 0;
	}

	friend std::ostream &operator<<(std::ostream &os, const Atom &atom);

  private:
	friend class Structure;
	void setID(int id);

	AtomImpl *impl();
	const AtomImpl *impl() const;

	struct AtomImpl *mImpl_;

	// cached values
	std::string mAtomID;
	std::string mCompID;
	std::string mAsymID;
	int mSeqID;
	std::string mAltID;
};

inline void swap(mmcif::Atom &a, mmcif::Atom &b)
{
	a.swap(b);
}

inline double Distance(const Atom &a, const Atom &b)
{
	return Distance(a.location(), b.location());
}

inline double DistanceSquared(const Atom &a, const Atom &b)
{
	return DistanceSquared(a.location(), b.location());
}

typedef std::vector<Atom> AtomView;

// --------------------------------------------------------------------

class Residue
{
  public:
	// constructors should be private, but that's not possible for now (needed in emplace)

	// constructor for waters
	Residue(const Structure &structure, const std::string &compoundID,
		const std::string &asymID, const std::string &authSeqID);

	// constructor for a residue without a sequence number
	Residue(const Structure &structure, const std::string &compoundID,
		const std::string &asymID);

	// constructor for a residue with a sequence number
	Residue(const Structure &structure, const std::string &compoundID,
		const std::string &asymID, int seqID, const std::string &authSeqID);

	Residue(const Residue &rhs) = delete;
	Residue &operator=(const Residue &rhs) = delete;

	Residue(Residue &&rhs);
	Residue &operator=(Residue &&rhs);

	virtual ~Residue();

	const Compound &compound() const;
	const AtomView &atoms() const;

	/// \brief Unique atoms returns only the atoms without alternates and the first of each alternate atom id.
	AtomView unique_atoms() const;

	/// \brief The alt ID used for the unique atoms
	std::string unique_alt_id() const;

	Atom atomByID(const std::string &atomID) const;

	const std::string &compoundID() const { return mCompoundID; }
	void setCompoundID(const std::string &id) { mCompoundID = id; }

	const std::string &asymID() const { return mAsymID; }
	int seqID() const { return mSeqID; }
	std::string entityID() const;

	std::string authAsymID() const;
	std::string authSeqID() const;
	std::string authInsCode() const;

	// return a human readable PDB-like auth id (chain+seqnr+iCode)
	std::string authID() const;

	// similar for mmCIF space
	std::string labelID() const;

	// Is this residue a single entity?
	bool isEntity() const;

	bool isWater() const { return mCompoundID == "HOH"; }

	const Structure &structure() const { return *mStructure; }

	bool empty() const { return mStructure == nullptr; }

	bool hasAlternateAtoms() const;

	/// \brief Return the list of unique alt ID's present in this residue
	std::set<std::string> getAlternateIDs() const;

	/// \brief Return the list of unique atom ID's
	std::set<std::string> getAtomIDs() const;

	/// \brief Return the list of atoms having ID \a atomID
	AtomView getAtomsByID(const std::string &atomID) const;

	// some routines for 3d work
	std::tuple<Point, float> centerAndRadius() const;

	friend std::ostream &operator<<(std::ostream &os, const Residue &res);

  protected:
	Residue() {}

	friend class Polymer;

	const Structure *mStructure = nullptr;
	std::string mCompoundID, mAsymID;
	int mSeqID = 0;

	// Watch out, this is used only to label waters... The rest of the code relies on
	// MapLabelToAuth to get this info. Perhaps we should rename this member field.
	std::string mAuthSeqID;
	AtomView mAtoms;
};

// --------------------------------------------------------------------
// a monomer models a single Residue in a protein chain

class Monomer : public Residue
{
  public:
	//	Monomer();
	Monomer(const Monomer &rhs) = delete;
	Monomer &operator=(const Monomer &rhs) = delete;

	Monomer(Monomer &&rhs);
	Monomer &operator=(Monomer &&rhs);

	Monomer(const Polymer &polymer, size_t index, int seqID, const std::string &authSeqID,
		const std::string &compoundID);

	bool is_first_in_chain() const;
	bool is_last_in_chain() const;

	// convenience
	bool has_alpha() const;
	bool has_kappa() const;

	// Assuming this is really an amino acid...

	float phi() const;
	float psi() const;
	float alpha() const;
	float kappa() const;
	float tco() const;
	float omega() const;

	// torsion angles
	size_t nrOfChis() const;
	float chi(size_t i) const;

	bool isCis() const;

	/// \brief Returns true if the four atoms C, CA, N and O are present
	bool isComplete() const;

	/// \brief Returns true if any of the backbone atoms has an alternate
	bool hasAlternateBackboneAtoms() const;

	Atom CAlpha() const { return atomByID("CA"); }
	Atom C() const { return atomByID("C"); }
	Atom N() const { return atomByID("N"); }
	Atom O() const { return atomByID("O"); }
	Atom H() const { return atomByID("H"); }

	bool isBondedTo(const Monomer &rhs) const
	{
		return this != &rhs and areBonded(*this, rhs);
	}

	static bool areBonded(const Monomer &a, const Monomer &b, float errorMargin = 0.5f);
	static bool isCis(const Monomer &a, const Monomer &b);
	static float omega(const Monomer &a, const Monomer &b);

	// for LEU and VAL
	float chiralVolume() const;

  private:
	const Polymer *mPolymer;
	size_t mIndex;
};

// --------------------------------------------------------------------

class Polymer : public std::vector<Monomer>
{
  public:
	Polymer(const Structure &s, const std::string &entityID, const std::string &asymID);

	Polymer(const Polymer &) = delete;
	Polymer &operator=(const Polymer &) = delete;

	//	Polymer(Polymer&& rhs) = delete;
	//	Polymer& operator=(Polymer&& rhs) = de;

	Monomer &getBySeqID(int seqID);
	const Monomer &getBySeqID(int seqID) const;

	Structure *structure() const { return mStructure; }

	std::string asymID() const { return mAsymID; }
	std::string entityID() const { return mEntityID; }

	std::string chainID() const;

	int Distance(const Monomer &a, const Monomer &b) const;

  private:
	Structure *mStructure;
	std::string mEntityID;
	std::string mAsymID;
	cif::RowSet mPolySeq;
};

// --------------------------------------------------------------------
// file is a reference to the data stored in e.g. the cif file.
// This object is not copyable.

class File : public std::enable_shared_from_this<File>
{
  public:
	File();
	File(const std::filesystem::path &path);
	File(const char *data, size_t length); // good luck trying to find out what it is...
	~File();

	File(const File &) = delete;
	File &operator=(const File &) = delete;

	cif::Datablock& createDatablock(const std::string_view name);

	void load(const std::filesystem::path &path);
	void save(const std::filesystem::path &path);

	Structure *model(size_t nr = 1);

	struct FileImpl &impl() const { return *mImpl; }

	cif::Datablock &data();
	cif::File &file();

  private:
	struct FileImpl *mImpl;
};

// --------------------------------------------------------------------

enum class StructureOpenOptions
{
	SkipHydrogen = 1 << 0
};

inline bool operator&(StructureOpenOptions a, StructureOpenOptions b)
{
	return static_cast<int>(a) bitand static_cast<int>(b);
}

// --------------------------------------------------------------------

class Structure
{
  public:
	Structure(File &p, size_t modelNr = 1, StructureOpenOptions options = {});
	Structure &operator=(const Structure &) = delete;
	~Structure();

	// Create a read-only clone of the current structure (for multithreaded calculations that move atoms)
	Structure(const Structure &);

	File &getFile() const;

	const AtomView &atoms() const { return mAtoms; }
	AtomView waters() const;

	const std::list<Polymer> &polymers() const { return mPolymers; }
	std::list<Polymer> &polymers() { return mPolymers; }

	const std::vector<Residue> &nonPolymers() const { return mNonPolymers; }
	const std::vector<Residue> &branchResidues() const { return mBranchResidues; }

	Atom getAtomByID(std::string id) const;
	// Atom getAtomByLocation(Point pt, float maxDistance) const;

	Atom getAtomByLabel(const std::string &atomID, const std::string &asymID,
		const std::string &compID, int seqID, const std::string &altID = "");

	/// \brief Return the atom closest to point \a p
	Atom getAtomByPosition(Point p) const;

	/// \brief Return the atom closest to point \a p with atom type \a type in a residue of type \a res_type
	Atom getAtomByPositionAndType(Point p, std::string_view type, std::string_view res_type) const;

	/// \brief Get a residue, if \a seqID is zero, the non-polymers are searched
	const Residue &getResidue(const std::string &asymID, const std::string &compID, int seqID = 0) const;

	/// \brief Get a residue, if \a seqID is zero, the non-polymers are searched
	Residue &getResidue(const std::string &asymID, const std::string &compID, int seqID = 0);

	/// \brief Get a the single residue for an asym with id \a asymID
	const Residue &getResidue(const std::string &asymID) const;

	/// \brief Get a the single residue for an asym with id \a asymID
	Residue &getResidue(const std::string &asymID);

	// map between auth and label locations

	std::tuple<std::string, int, std::string> MapAuthToLabel(const std::string &asymID,
		const std::string &seqID, const std::string &compID, const std::string &insCode = "");

	std::tuple<std::string, std::string, std::string, std::string> MapLabelToAuth(
		const std::string &asymID, int seqID, const std::string &compID);

	// returns chain, seqnr, icode
	std::tuple<char, int, char> MapLabelToAuth(
		const std::string &asymID, int seqID) const;

	// returns chain,seqnr,comp,iCode
	std::tuple<std::string, int, std::string, std::string> MapLabelToPDB(
		const std::string &asymID, int seqID, const std::string &compID,
		const std::string &authSeqID) const;

	std::tuple<std::string, int, std::string> MapPDBToLabel(
		const std::string &asymID, int seqID, const std::string &compID, const std::string &iCode) const;

	// Actions
	void removeAtom(Atom &a);
	void swapAtoms(Atom &a1, Atom &a2); // swap the labels for these atoms
	void moveAtom(Atom &a, Point p);    // move atom to a new location
	void changeResidue(Residue &res, const std::string &newCompound,
		const std::vector<std::tuple<std::string, std::string>> &remappedAtoms);

	/// \brief Create a new non-polymer entity, returns new ID
	/// \param mon_id	The mon_id for the new nonpoly, must be an existing and known compound from CCD
	/// \return			The ID of the created entity
	std::string createNonPolyEntity(const std::string &mon_id);

	/// \brief Create a new NonPolymer struct_asym with atoms constructed from \a atoms, returns asym_id.
	/// This method assumes you are copying data from one cif file to another.
	///
	/// \param entity_id	The entity ID of the new nonpoly
	/// \param atoms		The array of atom_site rows containing the data.
	/// \return				The newly create asym ID
	std::string createNonpoly(const std::string &entity_id, const std::vector<mmcif::Atom> &atoms);

	/// \brief To sort the atoms in order of model > asym-id > res-id > atom-id
	/// Will asssign new atom_id's to all atoms. Be carefull
	void sortAtoms();

	/// \brief Translate the coordinates of all atoms in the structure by \a t
	void translate(Point t);

	/// \brief Rotate the coordinates of all atoms in the structure by \a q
	void rotate(Quaternion t);

	const std::vector<Residue> &getNonPolymers() const { return mNonPolymers; }
	const std::vector<Residue> &getBranchResidues() const { return mBranchResidues; }

	void cleanupEmptyCategories();

	/// \brief Direct access to underlying data
	cif::Category &category(std::string_view name) const;
	cif::Datablock &datablock() const;

  private:
	friend Polymer;
	friend Residue;
	// friend residue_view;
	// friend residue_iterator;

	std::string insertCompound(const std::string &compoundID, bool isEntity);

	void loadData();
	void updateAtomIndex();

	void loadAtomsForModel(StructureOpenOptions options);

	File &mFile;
	size_t mModelNr;
	AtomView mAtoms;
	std::vector<size_t> mAtomIndex;
	std::list<Polymer> mPolymers;
	std::vector<Residue> mNonPolymers, mBranchResidues;
};

} // namespace mmcif
