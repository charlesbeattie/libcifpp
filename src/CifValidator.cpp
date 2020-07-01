// cif parsing library

#include <boost/algorithm/string.hpp>

//// since gcc's regex is crashing....
#include <boost/regex.hpp>
// #include <regex>

#include "cif++/Cif++.hpp"
#include "cif++/CifParser.hpp"
#include "cif++/CifValidator.hpp"

using namespace std;
namespace ba = boost::algorithm;

extern int VERBOSE;

namespace cif
{

ValidationError::ValidationError(const string& msg)
	: mMsg(msg)
{
}

ValidationError::ValidationError(const string& cat, const string& item, const string& msg)
	: mMsg("When validating _" + cat + '.' + item + ": " + msg)
{
}

// --------------------------------------------------------------------

DDL_PrimitiveType mapToPrimitiveType(const string& s)
{
	DDL_PrimitiveType result;
	if (iequals(s, "char"))
		result = ptChar;
	else if (iequals(s, "uchar"))
		result = ptUChar;
	else if (iequals(s, "numb"))
		result = ptNumb;
	else
		throw ValidationError("Not a known primitive type");
	return result;
}

// --------------------------------------------------------------------

int ValidateType::compare(const char* a, const char* b) const
{
	int result = 0;
	
	if (*a == 0)
		result = *b == 0 ? 0 : -1;
	else if (*b == 0)
		result = *a == 0 ? 0 : +1;
	else
	{
		try
		{
			switch (mPrimitiveType)
			{
				case ptNumb:
				{
					double da = strtod(a, nullptr);
					double db = strtod(b, nullptr);
					
					auto d = da - db;
					if (abs(d) > numeric_limits<double>::epsilon())
					{
						if (d > 0)
							result = 1;
						else if (d < 0)
							result = -1;
					}
					break;
				}
				
				case ptUChar:
				case ptChar:
				{
					// CIF is guaranteed to have ascii only, therefore this primitive code will do
					// also, we're collapsing spaces
					
					auto ai = a, bi = b;
					for (;;)
					{
						if (*ai == 0)
						{
							if (*bi != 0)
								result = -1;
							break;
						}
						else if (*bi == 0)
						{
							result = 1;
							break;
						}
						
						char ca = *ai;
						char cb = *bi;

						if (mPrimitiveType == ptUChar)
						{
							ca = toupper(ca);
							cb = toupper(cb);
						}
						
						result = ca - cb;
						
						if (result != 0)
							break;
						
						if (ca == ' ')
						{
							while (ai[1] == ' ')
								++ai;
							while (bi[1] == ' ')
								++bi;
						}
						
						++ai;
						++bi;
					}
					
					break;
				}
			}
		}
		catch (const std::invalid_argument& ex)
		{
			result = 1;
		}
	}
	
	return result;
}

// --------------------------------------------------------------------

//void ValidateItem::addLinked(ValidateItem* parent, const std::string& parentItem, const std::string& childItem)
//{
////	if (mParent != nullptr and VERBOSE)
////		cerr << "replacing parent in " << mCategory->mName << " from " << mParent->mCategory->mName << " to " << parent->mCategory->mName << endl;
////	mParent = parent;
//
//	if (mType == nullptr and parent != nullptr)
//		mType = parent->mType;
//		
//	if (parent != nullptr)
//	{
//		mLinked.push_back({parent, parentItem, childItem});
//
//		parent->mChildren.insert(this);
////	
////		if (mCategory->mKeys == vector<string>{mTag})
////			parent->mForeignKeys.insert(this);
//	}
//}

void ValidateItem::operator()(string value) const
{
	if (not value.empty() and value != "?" and value != ".")
	{
		if (mType != nullptr and not std::regex_match(value, mType->mRx))
			throw ValidationError(mCategory->mName, mTag, "Value '" + value + "' does not match type expression for type " + mType->mName);

		if (not mEnums.empty())
		{
			if (mEnums.count(value) == 0)
				throw ValidationError(mCategory->mName, mTag, "Value '" + value + "' is not in the list of allowed values");
		}
	}
}

// --------------------------------------------------------------------

void ValidateCategory::addItemValidator(ValidateItem&& v)
{
	if (v.mMandatory)
		mMandatoryFields.insert(v.mTag);

	v.mCategory = this;

	auto r = mItemValidators.insert(move(v));
	if (not r.second and VERBOSE >= 4)
		cout << "Could not add validator for item " << v.mTag << " to category " << mName << endl;
}

const ValidateItem* ValidateCategory::getValidatorForItem(string tag) const
{
	const ValidateItem* result = nullptr;
	auto i = mItemValidators.find(ValidateItem{tag});
	if (i != mItemValidators.end())
		result = &*i;
	else if (VERBOSE > 4)
		cout << "No validator for tag " << tag << endl;
	return result;
}

// --------------------------------------------------------------------

Validator::Validator()
{
}

Validator::~Validator()
{
}

void Validator::addTypeValidator(ValidateType&& v)
{
	auto r = mTypeValidators.insert(move(v));
	if (not r.second and VERBOSE > 4)
		cout << "Could not add validator for type " << v.mName << endl;
}

const ValidateType* Validator::getValidatorForType(string typeCode) const
{
	const ValidateType* result = nullptr;
	
	auto i = mTypeValidators.find(ValidateType{ typeCode, ptChar, std::regex() });
	if (i != mTypeValidators.end())
		result = &*i;
	else if (VERBOSE > 4)
		cout << "No validator for type " << typeCode << endl;
	return result;
}

void Validator::addCategoryValidator(ValidateCategory&& v)
{
	auto r = mCategoryValidators.insert(move(v));
	if (not r.second and VERBOSE > 4)
		cout << "Could not add validator for category " << v.mName << endl;
}

const ValidateCategory* Validator::getValidatorForCategory(string category) const
{
	const ValidateCategory* result = nullptr;
	auto i = mCategoryValidators.find(ValidateCategory{category});
	if (i != mCategoryValidators.end())
		result = &*i;
	else if (VERBOSE > 4)
		cout << "No validator for category " << category << endl;
	return result;
}

ValidateItem* Validator::getValidatorForItem(string tag) const
{
	ValidateItem* result = nullptr;
	
	string cat, item;
	std::tie(cat, item) = splitTagName(tag);

	auto* cv = getValidatorForCategory(cat);
	if (cv != nullptr)
		result = const_cast<ValidateItem*>(cv->getValidatorForItem(item));

	if (result == nullptr and VERBOSE > 4)
		cout << "No validator for item " << tag << endl;

	return result;
}

void Validator::addLinkValidator(ValidateLink&& v)
{
	assert(v.mParentKeys.size() == v.mChildKeys.size());
	if (v.mParentKeys.size() != v.mChildKeys.size())
		throw runtime_error("unequal number of keys for parent and child in link");
	
	auto pcv = getValidatorForCategory(v.mParentCategory);
	auto ccv = getValidatorForCategory(v.mChildCategory);
	
	if (pcv == nullptr)
		throw runtime_error("unknown parent category " + v.mParentCategory);

	if (ccv == nullptr)
		throw runtime_error("unknown child category " + v.mChildCategory);

	for (size_t i = 0; i < v.mParentKeys.size(); ++i)
	{
		auto piv = pcv->getValidatorForItem(v.mParentKeys[i]);
	
		if (piv == nullptr)
			throw runtime_error("unknown parent tag _" + v.mParentCategory + '.' + v.mParentKeys[i]);

		auto civ = ccv->getValidatorForItem(v.mChildKeys[i]);
		if (civ == nullptr)
			throw runtime_error("unknown child tag _" + v.mChildCategory + '.' + v.mChildKeys[i]);
		
		if (civ->mType == nullptr and piv->mType != nullptr)
			const_cast<ValidateItem*>(civ)->mType = piv->mType;
	}		
	
	mLinkValidators.emplace_back(move(v));
}

vector<const ValidateLink*> Validator::getLinksForParent(const string& category) const
{
	vector<const ValidateLink*> result;

	for (auto& l: mLinkValidators)
	{
		if (l.mParentCategory == category)
			result.push_back(&l);
	}
	
	return result;
}

vector<const ValidateLink*> Validator::getLinksForChild(const string& category) const
{
	vector<const ValidateLink*> result;

	for (auto& l: mLinkValidators)
	{
		if (l.mChildCategory == category)
			result.push_back(&l);
	}
	
	return result;
}

void Validator::reportError(const string& msg, bool fatal)
{
	if (mStrict or fatal)
		throw ValidationError(msg);
	else if (VERBOSE)
		cerr << msg << endl;
}

}
