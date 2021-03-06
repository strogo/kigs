#include "PrecompiledHeaders.h"
#include "CoreItemOperator.h"
#include "CoreModifiableAttribute.h"

#include <algorithm>


struct RemoveDelimiter
{
	bool operator()(char c)
	{
		return (c == '\r' || c == '\t' || c == ' ' || c == '\n');
	}
};


CoreItemEvaluationContext* CoreItemEvaluationContext::mCurrentContext = 0;
std::mutex	CoreItemEvaluationContext::mMutex;

void	CoreItemEvaluationContext::SetContext(CoreItemEvaluationContext* set)
{
	mMutex.lock();
	mCurrentContext = set;
}
void	CoreItemEvaluationContext::ReleaseContext()
{
	mCurrentContext = 0;
	mMutex.unlock();
}

template<typename operandType>
CoreItemSP	CoreItemOperator<operandType>::Construct(const kstl::string& formulae, CoreModifiable* target, kstl::vector<SpecificOperator>* specificList)
{
	kstl::string cleanFormulae = formulae;
	cleanFormulae.erase(std::remove_if(cleanFormulae.begin(), cleanFormulae.end(), RemoveDelimiter()), cleanFormulae.end());
	AsciiParserUtils	parser((char*)cleanFormulae.c_str(), cleanFormulae.length());

	ConstructContext	context;
	context.mTarget = target;
	context.mSpecificList = specificList;

	ConstructContextMap(context.mMap, specificList);

	CoreItemSP	result = Parse(parser, context);

	return result;

}

template<typename operandType>
CoreItemSP	CoreItemOperator<operandType>::Construct(const kstl::string& formulae, CoreModifiable* target, const kigs::unordered_map<kstl::string, CoreItemOperatorCreateMethod>&	lmap)
{
	kstl::string cleanFormulae = formulae;
	cleanFormulae.erase(std::remove_if(cleanFormulae.begin(), cleanFormulae.end(), RemoveDelimiter()), cleanFormulae.end());
	AsciiParserUtils	parser((char*)cleanFormulae.c_str(), cleanFormulae.length());

	ConstructContext	context;
	context.mTarget = target;
	context.mMap = lmap;
	context.mSpecificList = nullptr;

	CoreItemSP	result = Parse(parser, context);

	return result;

}

template<typename operandType>
void	CoreItemOperator<operandType>::ConstructContextMap(kigs::unordered_map<kstl::string, CoreItemOperatorCreateMethod>&	lmap, kstl::vector<SpecificOperator>* specificList)
{
	lmap.clear();
	
	lmap["sin"] = &SinusOperator<operandType>::create;
	lmap["cos"] = &CosinusOperator<operandType>::create;
	lmap["tan"] = &TangentOperator<operandType>::create;
	lmap["abs"] = &AbsOperator<operandType>::create;
	lmap["min"] = &MinOperator<operandType>::create;
	lmap["max"] = &MaxOperator<operandType>::create;
	lmap["if"] = &IfThenElseOperator<operandType>::create;

	// push specific
	if (specificList)
	{
		typename kstl::vector<SpecificOperator>::const_iterator itstart = specificList->begin();
		typename kstl::vector<SpecificOperator>::const_iterator itend = specificList->end();
		while (itstart != itend)
		{
			lmap[(*itstart).mKeyWord] = (*itstart).mCreateMethod;
			++itstart;
		}
	}
}

template<>
void	CoreItemOperator<kstl::string>::ConstructContextMap(kigs::unordered_map<kstl::string, CoreItemOperatorCreateMethod>& lmap, kstl::vector<SpecificOperator>* specificList)
{
	// nothing here
}

template<>
void	CoreItemOperator<Point2D>::ConstructContextMap(kigs::unordered_map<kstl::string, CoreItemOperatorCreateMethod>& lmap, kstl::vector<SpecificOperator>* specificList)
{
	// nothing here
}

template<>
void	CoreItemOperator<Point3D>::ConstructContextMap(kigs::unordered_map<kstl::string, CoreItemOperatorCreateMethod>& lmap, kstl::vector<SpecificOperator>* specificList)
{
	// nothing here
}

template<>
void	CoreItemOperator<Vector4D>::ConstructContextMap(kigs::unordered_map<kstl::string, CoreItemOperatorCreateMethod>& lmap, kstl::vector<SpecificOperator>* specificList)
{
	// nothing here
}



template<typename operandType>
CoreItemSP	CoreItemOperator<operandType>::Parse(AsciiParserUtils& formulae, ConstructContext& context)
{
	
	// check if real block or parameter to avoid things like (ajhgj)+(bjhb) considered as one block
	if (formulae[0] == '(') // is this a full block ?
	{
		if (formulae[formulae.length() - 1] == ')')
		{
			AsciiParserUtils	block(formulae);
			int oldpos = formulae.GetPosition();
			if (formulae.GetBlock(block, '(', ')'))
			{
				if (block.length() == (formulae.length() - 2))
				{
					CoreItemSP	op1 = Parse(block, context);
					return op1;
				}
				else
				{
					formulae.SetPosition(oldpos);
				}
			}
		}
	}



	if (formulae[0] == '[') // is this a modifiable method ?
	{
		if (formulae[formulae.length() - 1] == ']')
		{

			AsciiParserUtils	block(formulae);
			int oldpos = formulae.GetPosition();
			formulae.SetPosition(0);

			if (formulae.GetBlock(block, '[', ']'))
			{

				// test method

				AsciiParserUtils	paramblock(block);
				if (block.GetBlock(paramblock, '(', ')'))
				{
					formulae.SetPosition(1);
					AsciiParserUtils leading(formulae);
					formulae.GetString(leading, '(');
					// create a method
					CoreModifiableAttributeOperator<operandType>* opeattribute = new CoreModifiableAttributeOperator<operandType>((const kstl::string&)leading, context.mTarget);
					if (paramblock.length())
					{
						CoreItemSP	op1;
						kstl::vector<kstl::string>	params = FindFirstLevelParams(paramblock, context);

						kstl::vector<kstl::string>::iterator	itparambegin = params.begin();
						kstl::vector<kstl::string>::iterator	itparamend = params.end();

						while (itparambegin != itparamend)
						{
							kstl::string& currentParam = (*itparambegin);
							char* currentParamC = (char*)currentParam.c_str();

							AsciiParserUtils	param(currentParamC, currentParam.length());
							op1 = Parse(param, context);
							if (!op1.isNil())
							{
								opeattribute->push_back(op1);
							}
							++itparambegin;
						}
					}

					return CoreItemSP((CoreItem*)opeattribute, StealRefTag{});
				}

			}
			formulae.SetPosition(oldpos);
			
		}
	}


	if (formulae[0] == '{') // is this a vector
	{
		if (formulae[formulae.length() - 1] == '}')
		{
			AsciiParserUtils	block(formulae);
			int oldpos = formulae.GetPosition();
			formulae.SetPosition(0);

			if (formulae.GetBlock(block, '{', '}'))
			{

				// test method
				AsciiParserUtils	paramblock(block);
				if (paramblock.length())
				{


					kstl::vector<kstl::string>	params = FindFirstLevelParams(paramblock, context);

					if (params.size())
					{
						
						CoreVector* opeattribute = new CoreVector();
						CoreItemSP	op1;

						kstl::vector<kstl::string>::iterator	itparambegin = params.begin();
						kstl::vector<kstl::string>::iterator	itparamend = params.end();

						while (itparambegin != itparamend)
						{
							kstl::string& currentParam = (*itparambegin);
							char* currentParamC = (char*)currentParam.c_str();

							AsciiParserUtils	param(currentParamC, currentParam.length());
							// inside a vector, each param is a float
							op1 = CoreItemOperator<float>::Parse(param, context);
							if (!op1.isNil())
							{
								opeattribute->push_back(op1);
							}
							++itparambegin;
						}

						return CoreItemSP((CoreItem*)opeattribute, StealRefTag{});
					}
				}

			}
			formulae.SetPosition(oldpos);

		}
	}

	if (formulae[0] == '#') // is this a modifiable attribute ?
	{
		if (formulae[formulae.length() - 1] == '#')
		{
		
			AsciiParserUtils	block(formulae);
			int oldpos = formulae.GetPosition();
			formulae.SetPosition(1);
		
			if (formulae.GetString(block, '#'))
			{
				if (block.length() == (formulae.length() - 2))
				{
					CoreModifiableAttributeOperator<operandType>* opeattribute = new CoreModifiableAttributeOperator<operandType>((const kstl::string&)block, context.mTarget);
					return CoreItemSP((CoreItem*)opeattribute, StealRefTag{});
				}
			}

			formulae.SetPosition(oldpos);
			
		}
	}

	// first check for instruction separators
	kstl::vector<CoreItemOperatorStruct>	FirstLevelOperatorList = FindFirstLevelSeparator(formulae, context);

	if (FirstLevelOperatorList.size())
	{
		CoreVector* newOperator = new InstructionListOperator <operandType>();

		// push each separated instruction 
		AsciiParserUtils	remaining(formulae);
		int starting = 0;
		int i;
		for (i = 0; i < (int)FirstLevelOperatorList.size(); i++)
		{
			AsciiParserUtils	operand(remaining);
			remaining.SetPosition(FirstLevelOperatorList[i].mPos - 1 - starting);

			if (remaining.GetLeadingPart(operand))
			{
				CoreItemSP instruction = Parse(operand, context);
				if(!instruction.isNil())
					newOperator->push_back(instruction);
			}
			starting += remaining.GetPosition() + FirstLevelOperatorList[i].mSize;
			remaining.SetPosition(remaining.GetPosition() + FirstLevelOperatorList[i].mSize);
			remaining.GetTrailingPart(operand);

			remaining = operand;
		}

		// last one
		CoreItemSP instruction = Parse(remaining, context);
		if (!instruction.isNil())
			newOperator->push_back(instruction);

		return CoreItemSP((CoreItem*)newOperator, StealRefTag{});
	}
	formulae.Reset();
	// no first level separator, search operators now
	FirstLevelOperatorList = FindFirstLevelOperators(formulae,context);

	if (FirstLevelOperatorList.size() == 0) // leaf
	{

		// check starting character
		if (formulae[0] == '-') // is this a neg unary operator ?
		{
			if (!((formulae[1] >= '0') && (formulae[1] <= '9'))) // this is not a constant 
			{
				CoreItemSP	op1;
				AsciiParserUtils	operand(formulae);
				formulae.SetPosition(1);

				if (formulae.GetTrailingPart(operand))
				{
					op1 = Parse(operand, context);
				}

				if (op1)
				{
					// check where to add neg operator
					NegOperator<operandType>*   neg = (new NegOperator<operandType>());
					neg->push_back(op1);
					return  CoreItemSP((CoreItem*)neg, StealRefTag{});
				}
			}
		}
		else if (formulae[0] == '!') // is this a not unary operator ?
		{
			CoreItemSP	op1;
			AsciiParserUtils	operand(formulae);
			formulae.SetPosition(1);

			if (formulae.GetTrailingPart(operand))
			{
				op1 = Parse(operand, context);
			}

			if (op1)
			{
				// check where to add neg operator
				NotOperator*   neg = (new NotOperator());
				neg->push_back(op1);
				return  CoreItemSP((CoreItem*)neg, StealRefTag{});
			}
		}

		formulae.Reset();

		float value;
		if (formulae.ReadFloat(value)) // check for leaf constant
		{
			CoreValue<kfloat>*   corevalue = (new CoreValue<kfloat>());
			*corevalue = value;
			return CoreItemSP((CoreItem*)corevalue, StealRefTag{});
		}
		
		// try to match other keyword
		kstl::string matchkeywork = "";
		{
			AsciiParserUtils	word(formulae);
			formulae.GetWord(word, '(');
			matchkeywork = word.c_str();
		}
		if (matchkeywork != "")
		{

			CoreItemOperator<operandType>*	newfunction = 0;
			newfunction = getOperator(matchkeywork, context);

			if (newfunction)
			{
				AsciiParserUtils	operand(formulae);
				formulae.SetPosition(matchkeywork.size());

				CoreItemSP	op1;
				if (formulae.GetTrailingPart(operand))
				{
					AsciiParserUtils	paramblock(operand);

					operand.GetBlock(paramblock, '(', ')');

					if (paramblock.length())
					{

						kstl::vector<kstl::string>	params = FindFirstLevelParams(paramblock, context);

						kstl::vector<kstl::string>::iterator	itparambegin = params.begin();
						kstl::vector<kstl::string>::iterator	itparamend = params.end();

						while (itparambegin != itparamend)
						{
							kstl::string& currentParam=(*itparambegin);
							char* currentParamC = (char*)currentParam.c_str();

							AsciiParserUtils	param(currentParamC, currentParam.length());
							op1 = Parse(param, context);
							if (!op1.isNil())
							{
								newfunction->push_back(op1);
							}
							++itparambegin;
						}
					}
				}
				return CoreItemSP((CoreItem*)newfunction, StealRefTag{});
			}

			// try to find a variable with this name
			CoreItem* variable= (CoreItem *)getVariable(matchkeywork);
			if (variable)
			{
				return CoreItemSP((CoreItem*)variable, GetRefTag{});
			}
			/*// just set value as a string
			CoreValue < kstl::string > &   corevalue = *(new CoreValue< kstl::string>());
			corevalue = matchkeywork;
			return corevalue;
			*/
			// dynamic var
			variable = new DynamicVariableOperator<operandType>(matchkeywork);
			return CoreItemSP((CoreItem*)variable, StealRefTag{});
		}

	}
	else // create operator tree
	{
		CoreVector*	newOperator = 0;
		while (FirstLevelOperatorList.size())
		{
			// find higher level
	
			int foundpriority = -1;
			kstl::vector<CoreItemOperatorStruct>::iterator	itfound = FirstLevelOperatorList.end();
			kstl::vector<CoreItemOperatorStruct>::iterator	itcurrent = FirstLevelOperatorList.begin();
			kstl::vector<CoreItemOperatorStruct>::iterator	itend = FirstLevelOperatorList.end();

			int i = 0;
			int ifound = 0;

			while (itcurrent != itend)
			{
				if ((*itcurrent).mPriority>foundpriority)
				{
					foundpriority = (*itcurrent).mPriority;
					itfound = itcurrent;
					ifound = i;
				}
				++itcurrent;
				++i;
			}

			CoreItemOperatorStruct& current = (*itfound);

			switch (current.mOp)
			{
			case '*':
			{
				newOperator = new MultOperator < operandType>();
			}
			break;
			case '+':
			{
				newOperator = new AddOperator < operandType>();
			}
			break;
			case '-':
			{
				newOperator = new SubOperator < operandType>();
			}
			break;
			case '/':
			{
				newOperator = new DivOperator < operandType>();
			}
			break;
			case '=':
			{
				newOperator = new EqualOperator();
			}
			break;
			case 'd':
			{
				newOperator = new NotEqualOperator();
			}
			break;
			case 's':
			{
				newOperator = new SupEqualOperator();
			}
			break;
			case 'i':
			{
				newOperator = new InfEqualOperator();
			}
			break;
			case '>':
			{
				newOperator = new SupOperator();
			}
			break;
			case '<':
			{
				newOperator = new InfOperator();
			}
			break;
			case '&':
			{
				newOperator = new AndOperator();
			}
			break;
			case '|':
			{
				newOperator = new OrOperator();
			}
			break;
			case 'a': // affectation
			{
				newOperator = new AffectOperator<operandType>();
			}
			break;

			}

			newOperator->push_back(current.mOp1);
			newOperator->push_back(current.mOp2);

			// replace newOperator in previous and next 
			if (ifound > 0)
			{
				FirstLevelOperatorList[ifound - 1].mOp2 = CoreItemSP(newOperator, StealRefTag{});
			}
			if (ifound < ((int)FirstLevelOperatorList.size() - 1))
			{
				FirstLevelOperatorList[ifound + 1].mOp1 = CoreItemSP(newOperator, StealRefTag{});
			}

			FirstLevelOperatorList.erase(itfound);
		}
		return CoreItemSP((CoreItem*)newOperator, StealRefTag{});
	}


	return CoreItemSP(nullptr);
}

template<typename operandType>
CoreItemOperator<operandType>* CoreItemOperator<operandType>::getOperator(const kstl::string& keyword, ConstructContext& context)
{

	typename kigs::unordered_map<kstl::string, CoreItemOperatorCreateMethod>::const_iterator itfound = context.mMap.find(keyword);
	while (itfound != context.mMap.end())
	{
		return  (CoreItemOperator<operandType>*)((*itfound).second());
	}
	return 0;
}

template<typename operandType>
GenericRefCountedBaseClass* CoreItemOperator<operandType>::getVariable(const kstl::string& keyword)
{
	if (CoreItemEvaluationContext::GetContext())
	{
		auto itfound = CoreItemEvaluationContext::GetContext()->mVariableList.find(CharToID::GetID(keyword));
		if (itfound != CoreItemEvaluationContext::GetContext()->mVariableList.end())
		{
			return (*itfound).second;
		}
	}
	return nullptr;
}

template<typename operandType>
kstl::vector<kstl::string>	CoreItemOperator<operandType>::FindFirstLevelParams(AsciiParserUtils& block, ConstructContext& context)
{
	int currentPos = 0;
	int prevPos = 0;
	char	currentChar;

	kstl::vector<kstl::string>	paramList;
	paramList.clear();

	int BlockLevel = 0;

	while (block.ReadChar(currentChar))
	{
		if (currentChar == ',')
		{
			if (BlockLevel == 0)
			{
				paramList.push_back(block.subString(prevPos, currentPos - prevPos));
				prevPos = currentPos + 1;
			}
		}
		else if (currentChar == '(')
		{
			++BlockLevel;
		}
		else if (currentChar == ')')
		{
			BlockLevel--;
		}
		++currentPos;
	}

	paramList.push_back(block.subString(prevPos, currentPos - prevPos));

	return paramList;	
}

template<typename operandType>
bool	CoreItemOperator<operandType>::CheckAffectation(char prevChar, int priority, AsciiParserUtils& block, kstl::vector<CoreItemOperatorStruct>& OperatorList)
{

	if (prevChar == '=')
	{
		CoreItemOperatorStruct toAdd;
		toAdd.mOp = 'a';
		toAdd.mPos = block.GetPosition() - 1;
		toAdd.mOp1 = CoreItemSP(nullptr);
		toAdd.mOp2 = CoreItemSP(nullptr);
		toAdd.mPriority = priority;
		toAdd.mSize = 1;
		OperatorList.push_back(toAdd);
		return true;
	}
	return false;
}

// search ; separators to splt block 
template<typename operandType>
kstl::vector<CoreItemOperatorStruct>	CoreItemOperator<operandType>::FindFirstLevelSeparator(AsciiParserUtils& block, ConstructContext& context)
{
	char	currentChar, prevChar;

	kstl::vector<CoreItemOperatorStruct>	OperatorList;
	OperatorList.clear();

	int BlockLevel = 0;
	bool prevIsValid = false;
	bool insideAttribute = false;
	prevChar = 0;

	while (block.ReadChar(currentChar))
	{
		int priority = 0;
		bool isValid = true;
		switch (currentChar)
		{
		case ';':
		{
			if ((BlockLevel == 0) && prevIsValid)
			{
				CoreItemOperatorStruct toAdd;
				toAdd.mOp = currentChar;
				toAdd.mPos = block.GetPosition();
				toAdd.mOp1 = CoreItemSP(nullptr);
				toAdd.mOp2 = CoreItemSP(nullptr);
				toAdd.mPriority = priority;
				toAdd.mSize = 1;
				OperatorList.push_back(toAdd);
				isValid = false;
			}
		}
		break;
		case '(':
		{
			++BlockLevel;
		}
		break;
		case ')':
		{
			BlockLevel--;
		}
		break;
		case '[':
		{
			++BlockLevel;
		}
		break;
		case ']':
		{
			BlockLevel--;
		}
		break;
		case '{':
		{
			++BlockLevel;
		}
		break;
		case '}':
		{
			BlockLevel--;
		}
		break;
		case '#':
		{
			if (insideAttribute)
			{
				BlockLevel--;
				insideAttribute = false;
			}
			else
			{
				BlockLevel++;
				insideAttribute = true;
			}
		}
		break;
		// logical op
		case '=':
		{
			if ((BlockLevel == 0) && prevIsValid)
			{
				if (prevChar == '=')
				{
					isValid = false;
				}
				else if (prevChar == '!')
				{
					isValid = false;
				}
				else if (prevChar == '<')
				{
					isValid = false;
				}
				else if (prevChar == '>')
				{
					isValid = false;
				}
			}
		}
		break;
		case '<':
		case '>':
		{
			if ((BlockLevel == 0) && prevIsValid)
			{
				isValid = false;
			}
		}
		break;
		case '&':
		{
			if ((BlockLevel == 0) && prevIsValid)
			{
				if (prevChar == '&')
				{
					isValid = false;
				}
			}
		}
		break;
		case '|':
		{
			if ((BlockLevel == 0) && prevIsValid)
			{
				if (prevChar == '|')
				{
					isValid = false;
				}
			}
		}
		break;
		default:
		{
			
		}
		}
		prevChar = currentChar;
		prevIsValid = isValid;
	}

	return OperatorList;

}

template<typename operandType>
kstl::vector<CoreItemOperatorStruct>	CoreItemOperator<operandType>::FindFirstLevelOperators(AsciiParserUtils& block, ConstructContext& context)
{
	char	currentChar,prevChar;

	kstl::vector<CoreItemOperatorStruct>	OperatorList;
	OperatorList.clear();

	int BlockLevel=0;
	bool prevIsValid = false;
	bool insideAttribute = false;
	prevChar = 0;

	while (block.ReadChar(currentChar))
	{
		int priority = 0;
		bool isValid = true;
		switch (currentChar)
		{
		case '*':
			++priority; // * priority = 4
		case '/':
			++priority; // / priority = 3
		case '+':
			++priority; // + priority = 2
		case '-':
		{
			++priority; // - priority = 1
			
			if ((BlockLevel == 0) && prevIsValid)
			{
				bool unaryNeg = false;
				if (currentChar == '-') // Unary neg
				{
					unaryNeg=CheckAffectation(prevChar, 0, block, OperatorList); // affectation priority is 0 
				}
				if (!unaryNeg)
				{
					CoreItemOperatorStruct toAdd;
					toAdd.mOp = currentChar;
					toAdd.mPos = block.GetPosition();
					toAdd.mOp1 = CoreItemSP(nullptr);
					toAdd.mOp2 = CoreItemSP(nullptr);
					toAdd.mPriority = priority;
					toAdd.mSize = 1;
					OperatorList.push_back(toAdd);
					isValid = false;
				}
			}
		}
		break;
		case '(':
		{
			if ((BlockLevel == 0) && prevIsValid)
			{
				CheckAffectation(prevChar, 0, block, OperatorList); // affectation priority is 0 
			}
			++BlockLevel;
		}
		break;
		case ')':
		{
			BlockLevel--;
		}
		break;
		case '[':
		{
			if ((BlockLevel == 0) && prevIsValid)
			{
				CheckAffectation(prevChar, 0, block, OperatorList); // affectation priority is 0 
			}
			++BlockLevel;
		}
		break;
		case ']':
		{
			BlockLevel--;
		}
		break;
		case '{':
		{
			if ((BlockLevel == 0) && prevIsValid)
			{
				CheckAffectation(prevChar, 0, block, OperatorList); // affectation priority is 0 
			}
			++BlockLevel;
		}
		break;
		case '}':
		{
			BlockLevel--;
		}
		break;
		case '#':
		{
			if (insideAttribute)
			{
				BlockLevel--;
				insideAttribute = false;
			}
			else
			{
				if ((BlockLevel == 0) && prevIsValid)
				{
					CheckAffectation(prevChar, 0, block, OperatorList); // affectation priority is 0
				}
				
				BlockLevel++;
				insideAttribute = true;
			}
		}
		break;
		// logical op
		case '=':
		{
			if ((BlockLevel == 0) && prevIsValid)
			{
				if (prevChar == '=')
				{
					CoreItemOperatorStruct toAdd;
					toAdd.mOp = currentChar;
					toAdd.mPos = block.GetPosition()-1;
					toAdd.mOp1 = CoreItemSP(nullptr);
					toAdd.mOp2 = CoreItemSP(nullptr);
					toAdd.mPriority = priority;
					toAdd.mSize = 2;
					OperatorList.push_back(toAdd);
					isValid = false;
				}
				else if (prevChar == '!')
				{
					CoreItemOperatorStruct toAdd;
					toAdd.mOp = 'd';
					toAdd.mPos = block.GetPosition()-1;
					toAdd.mOp1 = CoreItemSP(nullptr);
					toAdd.mOp2 = CoreItemSP(nullptr);
					toAdd.mPriority = priority;
					toAdd.mSize = 2;
					OperatorList.push_back(toAdd);
					isValid = false;
				}
				else if (prevChar == '<') 
				{
					// change previous
					OperatorList[OperatorList.size() - 1].mOp = 'i';
					OperatorList[OperatorList.size() - 1].mSize = 2;
					isValid = false;
				}
				else if (prevChar == '>')
				{
					// change previous
					OperatorList[OperatorList.size() - 1].mOp = 's';
					OperatorList[OperatorList.size() - 1].mSize = 2;
					isValid = false;
				}
			}
		}
		break;
		case '<':
		case '>':
		{
			if ((BlockLevel == 0) && prevIsValid)
			{
				CoreItemOperatorStruct toAdd;
				toAdd.mOp = currentChar;
				toAdd.mPos = block.GetPosition();
				toAdd.mOp1 = CoreItemSP(nullptr);
				toAdd.mOp2 = CoreItemSP(nullptr);
				toAdd.mPriority = priority;
				toAdd.mSize = 1;
				OperatorList.push_back(toAdd);
				isValid = false;
			}
		}
		break;
		case '&':
		{
			if ((BlockLevel == 0) && prevIsValid)
			{
				if (prevChar == '&')
				{
					CoreItemOperatorStruct toAdd;
					toAdd.mOp = currentChar;
					toAdd.mPos = block.GetPosition()-1;
					toAdd.mOp1 = CoreItemSP(nullptr);
					toAdd.mOp2 = CoreItemSP(nullptr);
					toAdd.mPriority = priority;
					toAdd.mSize = 2;
					OperatorList.push_back(toAdd);
					isValid = false;
				}
			}
		}
		break;
		case '|':
		{
			if ((BlockLevel == 0) && prevIsValid)
			{
				if (prevChar == '|')
				{
					CoreItemOperatorStruct toAdd;
					toAdd.mOp = currentChar;
					toAdd.mPos = block.GetPosition() - 1;
					toAdd.mOp1 = CoreItemSP(nullptr);
					toAdd.mOp2 = CoreItemSP(nullptr);
					toAdd.mPriority = priority;
					toAdd.mSize = 2;
					OperatorList.push_back(toAdd);
					isValid = false;
				}
			}
		}
		break;
		default:
		{
			// check if previous was a '=' affect operator
			if ((BlockLevel == 0) && prevIsValid)
			{
				CheckAffectation(prevChar, 0, block, OperatorList); // affectation priority is 0
			}
		}
		}
		prevChar = currentChar;
		prevIsValid = isValid;
	}

	// if size, init operand
	if (OperatorList.size())
	{
		AsciiParserUtils	remaining(block);
		int starting = 0;
		int i;
		for (i = 0; i < (int)OperatorList.size(); i++)
		{
			AsciiParserUtils	operand(remaining);
			remaining.SetPosition(OperatorList[i].mPos - 1-starting);
			
			if (remaining.GetLeadingPart(operand))
			{
				OperatorList[i].mOp1 = Parse(operand, context);
			}
			starting += remaining.GetPosition() + OperatorList[i].mSize;
			remaining.SetPosition(remaining.GetPosition() + OperatorList[i].mSize);
			remaining.GetTrailingPart(operand);

			remaining = operand;
		}

		// last one
		OperatorList[OperatorList.size() - 1].mOp2 = Parse(remaining,context);
	
		// set op2
		for (i = 0; i < ((int)OperatorList.size())-1; i++)
		{
			OperatorList[i].mOp2 = OperatorList[i+1].mOp1;
		}
	}

	return OperatorList;

}

template<typename T>
void CoreModifiableAttributeOperator<T>::GetAttribute() const
{
	if (mAttributePath != "")
	{
		// search attribute
		kstl::string modifiablename;
		kstl::string attributename;

		CoreModifiableAttribute::ParseAttributePath(mAttributePath, modifiablename, attributename);

		if ((modifiablename == "") && (attributename == ""))
		{
			attributename = mAttributePath;
		}

		if (attributename != "")
		{
			CoreModifiable*	Owner = (CoreModifiable*)mTarget;
			if (modifiablename != "")
			{
				if (mTarget)
				{
					Owner = mTarget->GetInstanceByPath(modifiablename).get();
				}
				else
				{
					// global path, remove "/"
					if (modifiablename[0] == '/')
					{
						modifiablename = modifiablename.substr(1, modifiablename.length() - 1);
					}
					Owner = CoreModifiable::GetInstanceByGlobalPath(modifiablename).get();
				}
			}
			if (Owner)
			{
				// fake const
				CoreModifiableAttributeOperator<T>* other = (CoreModifiableAttributeOperator<T>*)(this);

				int attrindex = -1;
				// look for .x .y .z or .w at the end of attribute name
				if (attributename[attributename.size() - 2] == '.')
				{
					kstl::string extension = attributename.substr(attributename.size() - 2, 2);
					attributename = attributename.substr(0, attributename.size() - 2);
					switch (extension[1])
					{
					case'x':
						attrindex = 0;
						break;
					case'y':
						attrindex = 1;
						break;
					case'z':
						attrindex = 2;
						break;
					case'w':
						attrindex = 3;
						break;
					}
				}

				other->mAttribute = Owner->getAttribute(attributename);

				if (mAttribute)
				{

					if ((mAttribute->size() > 1) && (attrindex >= 0))
					{
						if (attrindex > (mAttribute->size() - 1))
						{
							attrindex = mAttribute->size() - 1;
						}
						other->mArrayAttributeIndex = attrindex;
					}
					other->mIsMethod = 0;
				}
				else if (Owner->HasMethod(attributename))// check for method
				{
					unsigned int id = CharToID::GetID(attributename);
					other->mIsMethod = 1;
					other->mMethodID = id;

					// for method, change target
					other->mTarget = Owner;
				}
			}
		}
	}

}


template<>
CoreModifiableAttributeOperator<kfloat>::operator kfloat() const
{
	kfloat	result = 0.0f;
	if ((!mAttribute) && (!mIsMethod))
	{
		GetAttribute();
	}
	if (mIsMethod == 0)
	{
		if (mAttribute)
		{
			if (mArrayAttributeIndex >= 0)
			{
				mAttribute->getArrayElementValue(result, 0, mArrayAttributeIndex);
			}
			else
			{
				mAttribute->getValue(result);
			}
		}
	}
	else
	{
		// push attributes
		kstl::vector<CoreModifiableAttribute*>	attributes;
		kstl::vector<CoreItemSP>::const_iterator itOperand = CoreVector::mVector.begin();
		kstl::vector<CoreItemSP>::const_iterator itOperandEnd = CoreVector::mVector.end();

		
		while (itOperand != itOperandEnd)
		{
			CoreModifiableAttribute* attribute = ((CoreItem*)(*itOperand).get())->createAttribute(mTarget);

			if (!attribute)
			{
				kfloat val = (kfloat)(*(*itOperand).get());
				attribute = new maFloat(*mTarget, false, LABEL_AND_ID(Val), val);
			}
			attributes.push_back(attribute);

			itOperand++;
		}

		// check if method adds an attribute
		int attrCount = attributes.size();

		// check if current context has mSender or data
		CoreModifiable* sendervariable = (CoreModifiable*)getVariable("sender");
		void* datavariable = (void*)getVariable("data");
		mTarget->CallMethod(mMethodID, attributes, datavariable, sendervariable);
	
		if (attributes.size() > attrCount)
		{
			 attributes.back()->getValue(result);
		}

		kstl::vector<CoreModifiableAttribute*>::iterator itattr = attributes.begin();
		kstl::vector<CoreModifiableAttribute*>::iterator itattrEnd = attributes.end();

		// delete attributes and set result
		while (itattr != itattrEnd)
		{
			delete (*itattr);

			itattr++;
		}

		attributes.clear();
		
	}

	return result;
}

template<>
CoreModifiableAttributeOperator<kstl::string>::operator kstl::string() const
{
	kstl::string	result = "";
	if ((!mAttribute) && (!mIsMethod))
	{
		GetAttribute();
	}
	if (mIsMethod == 0)
	{
		if (mAttribute)
		{
			if (mArrayAttributeIndex >= 0)
			{
				mAttribute->getArrayElementValue(result, 0, mArrayAttributeIndex);
			}
			else
			{
				mAttribute->getValue(result);
			}
		}
	}
	else
	{
		// push attributes
		kstl::vector<CoreModifiableAttribute*>	attributes;
		kstl::vector<CoreItemSP>::const_iterator itOperand = CoreVector::mVector.begin();
		kstl::vector<CoreItemSP>::const_iterator itOperandEnd = CoreVector::mVector.end();


		while (itOperand != itOperandEnd)
		{
			CoreModifiableAttribute* attribute = ((CoreItem*)(*itOperand).get())->createAttribute(mTarget);

			if (!attribute)
			{
				kstl::string val = (kstl::string)(*(*itOperand).get());
				attribute = new maString(*mTarget, false, LABEL_AND_ID(Val), val);
			}
			attributes.push_back(attribute);

			itOperand++;
		}

		// check if method adds an attribute
		int attrCount = attributes.size();

		// check if current context has mSender or data
		CoreModifiable* sendervariable = (CoreModifiable*)getVariable("sender");
		void* datavariable = (void*)getVariable("data");
		mTarget->CallMethod(mMethodID, attributes, datavariable, sendervariable);

		if (attributes.size() > attrCount)
		{
			attributes.back()->getValue(result);
		}


		kstl::vector<CoreModifiableAttribute*>::iterator itattr = attributes.begin();
		kstl::vector<CoreModifiableAttribute*>::iterator itattrEnd = attributes.end();

		// delete attributes and set result
		while (itattr != itattrEnd)
		{
			delete (*itattr);

			itattr++;
		}

		attributes.clear();

	}

	return result;
}


template<>
CoreModifiableAttributeOperator<Point2D>::operator Point2D() const
{
	Point2D	result(0.0f,0.0f);
	if ((!mAttribute) && (!mIsMethod))
	{
		GetAttribute();
	}
	if (mIsMethod == 0)
	{
		if (mAttribute)
		{
			if (mArrayAttributeIndex >= 0)
			{
				mAttribute->getArrayElementValue(result[mArrayAttributeIndex], 0, mArrayAttributeIndex);
			}
			else
			{
				mAttribute->getValue(result);
			}
		}
	}
	else
	{
		// push attributes
		kstl::vector<CoreModifiableAttribute*>	attributes;
		kstl::vector<CoreItemSP>::const_iterator itOperand = CoreVector::mVector.begin();
		kstl::vector<CoreItemSP>::const_iterator itOperandEnd = CoreVector::mVector.end();


		while (itOperand != itOperandEnd)
		{
			CoreModifiableAttribute* attribute = ((CoreItem*)(*itOperand).get())->createAttribute(mTarget);
			
			if (!attribute)
			{
				Point2D val((Point2D)(*itOperand));
				((*itOperand).get())->getValue(val);
				attribute = new maVect2DF(*mTarget, false, LABEL_AND_ID(Val), &(val.x));
			}

			attributes.push_back(attribute);

			itOperand++;
		}

		// check if method adds an attribute
		int attrCount = attributes.size();

		// check if current context has mSender or data
		CoreModifiable* sendervariable = (CoreModifiable*)getVariable("sender");
		void* datavariable = (void*)getVariable("data");
		mTarget->CallMethod(mMethodID, attributes, datavariable, sendervariable);

		if (attributes.size() > attrCount)
		{
			attributes.back()->getValue(result);
		}


		kstl::vector<CoreModifiableAttribute*>::iterator itattr = attributes.begin();
		kstl::vector<CoreModifiableAttribute*>::iterator itattrEnd = attributes.end();

		// delete attributes and set result
		while (itattr != itattrEnd)
		{
			delete (*itattr);

			itattr++;
		}

		attributes.clear();

	}

	return result;
}



template<>
CoreModifiableAttributeOperator<Point3D>::operator Point3D() const
{
	Point3D	result(0.0f, 0.0f,0.0f);
	if ((!mAttribute)&&(!mIsMethod))
	{
		GetAttribute();
	}
	if (mIsMethod == 0)
	{
		if (mAttribute)
		{
			if (mArrayAttributeIndex >= 0)
			{
				mAttribute->getArrayElementValue(result[mArrayAttributeIndex], 0, mArrayAttributeIndex);
			}
			else
			{
				mAttribute->getValue(result);
			}
		}
	}
	else
	{
		// push attributes
		kstl::vector<CoreModifiableAttribute*>	attributes;
		kstl::vector<CoreItemSP>::const_iterator itOperand = CoreVector::mVector.begin();
		kstl::vector<CoreItemSP>::const_iterator itOperandEnd = CoreVector::mVector.end();


		while (itOperand != itOperandEnd)
		{
			CoreModifiableAttribute* attribute = ((CoreItem*)(*itOperand).get())->createAttribute(mTarget);

			if (!attribute)
			{
				Point3D val = (Point3D)(*itOperand);
				attribute = new maVect3DF(*mTarget, false, LABEL_AND_ID(Val), &(val.x));
			}
			attributes.push_back(attribute);

			itOperand++;
		}

		// check if method adds an attribute
		int attrCount = attributes.size();

		// check if current context has mSender or data
		CoreModifiable* sendervariable = (CoreModifiable*)getVariable("sender");
		void* datavariable = (void*)getVariable("data");
		mTarget->CallMethod(mMethodID, attributes, datavariable, sendervariable);

		if (attributes.size() > attrCount)
		{
			attributes.back()->getValue(result);
		}


		kstl::vector<CoreModifiableAttribute*>::iterator itattr = attributes.begin();
		kstl::vector<CoreModifiableAttribute*>::iterator itattrEnd = attributes.end();

		// delete attributes and set result
		while (itattr != itattrEnd)
		{
			delete (*itattr);

			itattr++;
		}

		attributes.clear();

	}

	return result;
}


template<>
CoreModifiableAttributeOperator<Vector4D>::operator Vector4D() const
{
	Vector4D	result(0.0f, 0.0f, 0.0f,0.0f);
	if ((!mAttribute) && (!mIsMethod))
	{
		GetAttribute();
	}
	if (mIsMethod == 0)
	{
		if (mAttribute)
		{
			if (mArrayAttributeIndex >= 0)
			{
				mAttribute->getArrayElementValue(result[mArrayAttributeIndex], 0, mArrayAttributeIndex);
			}
			else
			{
				mAttribute->getValue(result);
			}
		}
	}
	else
	{
		// push attributes
		kstl::vector<CoreModifiableAttribute*>	attributes;
		kstl::vector<CoreItemSP>::const_iterator itOperand = CoreVector::mVector.begin();
		kstl::vector<CoreItemSP>::const_iterator itOperandEnd = CoreVector::mVector.end();


		while (itOperand != itOperandEnd)
		{
			CoreModifiableAttribute* attribute = ((CoreItem*)(*itOperand).get())->createAttribute(mTarget);

			if (!attribute)
			{
				Vector4D val((*itOperand)->operator Vector4D());
				attribute = new maVect4DF(*mTarget, false, LABEL_AND_ID(Val), &(val.x));
			}
			attributes.push_back(attribute);

			itOperand++;
		}

		// check if method adds an attribute
		int attrCount = attributes.size();

		// check if current context has mSender or data
		CoreModifiable* sendervariable = (CoreModifiable*)getVariable("sender");
		void* datavariable = (void*)getVariable("data");
		mTarget->CallMethod(mMethodID, attributes, datavariable, sendervariable);

		if (attributes.size() > attrCount)
		{
			attributes.back()->getValue(result);
		}


		kstl::vector<CoreModifiableAttribute*>::iterator itattr = attributes.begin();
		kstl::vector<CoreModifiableAttribute*>::iterator itattrEnd = attributes.end();

		// delete attributes and set result
		while (itattr != itattrEnd)
		{
			delete (*itattr);

			itattr++;
		}

		attributes.clear();

	}

	return result;
}

template<typename operandType>
CoreItem& CoreModifiableAttributeOperator<operandType>::operator=(const operandType& other)
{
	if ((!mAttribute) && (!mIsMethod))
	{
		GetAttribute();
	}
	if (mIsMethod == 0)
	{
		if (mAttribute)
		{
			if (mArrayAttributeIndex >= 0)
			{
				mAttribute->setArrayElementValue(other, 0, mArrayAttributeIndex);
			}
			else
			{
				// set value
				mAttribute->setValue(other);
			}
		}
	}
	return *this;
}

template<>
CoreItem& CoreModifiableAttributeOperator<Point2D>::operator=(const Point2D& other)
{
	if ((!mAttribute) && (!mIsMethod))
	{
		GetAttribute();
	}
	if (mIsMethod == 0)
	{
		if (mAttribute)
		{
			if (mArrayAttributeIndex >= 0)
			{
				mAttribute->setArrayElementValue(other[mArrayAttributeIndex], 0, mArrayAttributeIndex);
			}
			else
			{
				mAttribute->setValue(other);
			}
		}
	}
	return *this;
}

template<>
CoreItem& CoreModifiableAttributeOperator<Point3D>::operator=(const Point3D& other)
{
	if ((!mAttribute) && (!mIsMethod))
	{
		GetAttribute();
	}
	if (mIsMethod == 0)
	{
		if (mAttribute)
		{
			if (mArrayAttributeIndex >= 0)
			{
				mAttribute->setArrayElementValue(other[mArrayAttributeIndex], 0, mArrayAttributeIndex);
			}
			else
			{
				mAttribute->setValue(other);
			}
		}
	}
	return *this;
}

template<>
CoreItem& CoreModifiableAttributeOperator<Vector4D>::operator=(const Vector4D& other)
{
	if ((!mAttribute) && (!mIsMethod))
	{
		GetAttribute();
	}
	if (mIsMethod == 0)
	{
		if (mAttribute)
		{
			if (mArrayAttributeIndex >= 0)
			{
				mAttribute->setArrayElementValue(other[mArrayAttributeIndex], 0, mArrayAttributeIndex);
			}
			else
			{
				mAttribute->setValue(other);
			}
		}
	}
	return *this;
}


template<>
DynamicVariableOperator<kstl::string>::operator kstl::string() const
{
	CoreItem* var = (CoreItem * )getVariable(mVarName);
	if (var)
	{
		return (kstl::string)(*var);
	}
	return mVarName;
}


template<>
DynamicVariableOperator<kfloat>::operator kfloat() const
{
	CoreItem* var = (CoreItem*)getVariable(mVarName);
	if (var)
	{
		return (kfloat)(*var);
	}

	// atof

	return (kfloat)atof(mVarName.c_str());
}

template<>
DynamicVariableOperator<Point2D>::operator Point2D() const
{
	CoreItem* var = (CoreItem*)getVariable(mVarName);
	if (var)
	{
		return (Point2D)(*var);
	}
	return Point2D(0,0);
}

template<>
DynamicVariableOperator<Point3D>::operator Point3D() const
{
	CoreItem* var = (CoreItem*)getVariable(mVarName);
	if (var)
	{
		return (Point3D)(*var);
	}
	return Point3D(0, 0,0);
}

template<>
DynamicVariableOperator<Vector4D>::operator Vector4D() const
{
	CoreItem* var = (CoreItem*)getVariable(mVarName);
	if (var)
	{
		return var->operator Vector4D();
	}
	return Vector4D(0, 0, 0,0);
}


//template class CoreItemOperator<int>;
template class CoreItemOperator<kfloat>;
template class CoreItemOperator<kstl::string>;
template class CoreItemOperator<Point2D>;
template class CoreItemOperator<Point3D>;
template class CoreItemOperator<Vector4D>;