/*
 
 HyPhy - Hypothesis Testing Using Phylogenies.
 
 Copyright (C) 1997-now
 Core Developers:
 Sergei L Kosakovsky Pond (spond@ucsd.edu)
 Art FY Poon    (apoon@cfenet.ubc.ca)
 Steven Weaver (sweaver@ucsd.edu)
 
 Module Developers:
 Lance Hepler (nlhepler@gmail.com)
 Martin Smith (martin.audacis@gmail.com)
 
 Significant contributions from:
 Spencer V Muse (muse@stat.ncsu.edu)
 Simon DW Frost (sdf22@cam.ac.uk)
 
 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 */

#include "defines.h"
#include "variablecontainer.h"
#include "operation.h"

#include "likefunc.h"
#include "parser.h"
#include "polynoml.h"
#include "batchlan.h"


//__________________________________________________________________________________

_VariableContainer::_VariableContainer (void)
{
    theParent = nil;
    theModel = -1;
    iVariables = nil;
    dVariables = nil;
    gVariables = nil;
}

//__________________________________________________________________________________

void    _VariableContainer::Duplicate (BaseRef theO)
{
    _Variable::Duplicate (theO);
    _VariableContainer *theVC = (_VariableContainer*)theO;
    theParent= theVC->theParent;
    theModel = theVC->theModel;
    if (theVC->iVariables) {
        if (iVariables) {
            iVariables->Clear();
        } else {
            checkPointer(iVariables = new _SimpleList);
        }
        iVariables->Duplicate (theVC->iVariables);
    } else {
        if (iVariables) {
            delete (iVariables);
            iVariables = nil;
        }
    }
    if (theVC->dVariables) {
        if (dVariables) {
            dVariables->Clear();
        } else {
            checkPointer(dVariables = new _SimpleList);
        }
        dVariables->Duplicate (theVC->dVariables);
    } else {
        if (dVariables) {
            delete (dVariables);
            dVariables = nil;
        }
    }
    if (theVC->gVariables) {
        if (gVariables) {
            gVariables->Clear();
        } else {
            checkPointer (gVariables = new _SimpleList);
        }
        gVariables->Duplicate (theVC->gVariables);
    } else {
        if (gVariables) {
            delete (gVariables);
            gVariables = nil;
        }
    }
}


//__________________________________________________________________________________

void    _VariableContainer::TrimMemory ()
{
    if (iVariables) {
        iVariables->TrimMemory();
    }
    if (dVariables) {
        dVariables->TrimMemory();
    }
    if (gVariables) {
        gVariables->TrimMemory();
    }
}

//__________________________________________________________________________________

BaseRef _VariableContainer::makeDynamic (void)
{
    _VariableContainer * res = new _VariableContainer;
    checkPointer(res);
    memcpy ((char*)res, (char*)this, sizeof (_VariableContainer)); // ???
    res->Duplicate(this);
    return res;
}

//__________________________________________________________________________________

BaseRef _VariableContainer::toStr (void)
{
    _String * res = new _String (128L,true);

    checkPointer (res);

    (*res) << "Container Class:";
    (*res) << theName;
    (*res) << ":{ Independent Variables:";

    if (iVariables)
        for (long i = 0; i<iVariables->lLength; i+=2) {
            _String* s = (_String*)variablePtrs(iVariables->lData[i])->toStr();
            (*res) << s;
#ifndef USE_POINTER_VC
            if (i<independentVars.lLength-1)
#else
            if (i<iVariables->lLength-2)
#endif
                (*res) << ',';
            DeleteObject(s);
        }

    (*res) << "; Dependent Variables:";

    if (dVariables)
        for (long i2 = 0; i2<dVariables->lLength; i2+=2) {
            _String* s = (_String*)variablePtrs(dVariables->lData[i2])->toStr();
            (*res) << s;
#ifndef USE_POINTER_VC
            if (i2<independentVars.lLength-1)
#else
            if (i2<dVariables->lLength-2)
#endif
                (*res) << ',';
            DeleteObject(s);
        }

    (*res) << '}';
    res->Finalize();
    return res;
}

//__________________________________________________________________________________

_VariableContainer::_VariableContainer (_String theName, _String theTmplt, _VariableContainer* theP)
{
    iVariables = nil;
    dVariables = nil;
    gVariables = nil;
    InitializeVarCont (theName, theTmplt,theP);
}

//__________________________________________________________________________________

bool _VariableContainer::HasExplicitFormModel (void)
{
    if (theModel == -1) {
        return false;
    }
    return (modelTypeList.lData[theModel]);
}

//__________________________________________________________________________________

_Formula* _VariableContainer::GetExplicitFormModel (void)
{
    if (theModel == -1) {
        return nil;
    }
    if (modelTypeList.lData[theModel]) { // an explicit formula based matrix
        return (_Formula*)modelMatrixIndices.lData[theModel];
    }
    return nil;
}


//__________________________________________________________________________________

_Matrix* _VariableContainer::GetModelMatrix (void)
{
    if (theModel == -1) {
        return nil;
    }

    if (modelTypeList.lData[theModel]) { // an explicit formula based matrix
        return (_Matrix*) ((_Formula*)modelMatrixIndices.lData[theModel])->Compute();
    }

    return (_Matrix*) (LocateVar(modelMatrixIndices.lData[theModel])->GetValue());
}

//__________________________________________________________________________________

long _VariableContainer::GetModelDimension (void)
{
    long matrixDim = 0;
    if (theModel >= 0) {
        matrixDim = modelTypeList.lData[theModel];
        if (matrixDim == 0) {
            return GetModelMatrix()->GetHDim();
        }
    }
    return matrixDim;
}

//__________________________________________________________________________________

_Matrix* _VariableContainer::GetFreqMatrix (void)
{
    if (theModel>=0) {
        long freqID = modelFrequenciesIndices.lData[theModel];
        if (freqID>=0) {
            return (_Matrix*) (LocateVar(freqID)->GetValue());
        } else {
            return (_Matrix*) (LocateVar(-freqID-1)->GetValue());
        }
    }
    return nil;
}

//__________________________________________________________________________________
void    _VariableContainer::ScanModelBasedVariables (_String& fullName, _AVLListXL* varCache)
{
    if (theModel!= HY_NO_MODEL) { // build the matrix variables
        _SimpleList       mVars;
        _String           varName;
        
        {
            
            long cachedID = -1;
            bool doScan   = !varCache || (cachedID = varCache->Find ((BaseRef) theModel)) < 0 ;

            if (doScan) {

                _AVLList                ma (&mVars);
                ScanModelForVariables   (GetModelIndex(), ma,true,theModel,false);

                long freqID     = modelFrequenciesIndices.lData[theModel];
                if (freqID>=0) {
                    ((_Matrix*) (LocateVar(freqID)->GetValue()))->ScanForVariables2(ma,true,-1,false);
                }

                ma.ReorderList();

                if (varCache) {
                    varCache->Insert ((BaseRef)theModel, (long)mVars.makeDynamic(),false);
                }
            } else if (varCache) {
                mVars.Duplicate (varCache->GetXtra (cachedID));
            }

        }

        for (long i=0; i<mVars.lLength; i++) {
            _Variable * aVar = (_Variable*)variablePtrs (mVars.lData[i]);
            if (aVar->IsGlobal()) {
                //if (curVar->IsIndependent())
                {
                    if (!gVariables) {
                        checkPointer (gVariables = new _SimpleList);
                    }
                    (*gVariables) << aVar->GetAVariable();
                }
                continue;
            }

            long f = aVar->theName->FindBackwards('.',0,-1);
            if (f>=0) {
                varName = fullName&'.'& aVar->theName->Cut(f+1,-1);
            } else {
                varName = fullName&'.'& *aVar->theName;
            }


            f = LocateVarByName (varName);
            if (f<0) {
                _Variable v (varName);
                f = v.theIndex;
            } else {
                f = variableNames.GetXtra (f);
            }

            _Variable * spawnedVar = FetchVar (f);
            spawnedVar->SetBounds (aVar->GetLowerBound(), aVar->GetUpperBound());

            if (aVar->IsIndependent()) {
                if (!iVariables) {
                    checkPointer (iVariables = new _SimpleList);
                }
                (*iVariables) << f;
                (*iVariables) << mVars.lData[i];
            } else {
                if (!dVariables) {
                    checkPointer (dVariables = new _SimpleList);
                }
                (*dVariables) << f;
                (*dVariables) << mVars.lData[i];
            }
        }
    }
}

//__________________________________________________________________________________
void    _VariableContainer::SetModel (long modelID, _AVLListXL* varCache)
{
    theModel = modelID;
    ScanModelBasedVariables (*theName, varCache);
    SortVars();
}

//__________________________________________________________________________________
void    _VariableContainer::InitializeVarCont (_String& aName, _String& theTmplt, _VariableContainer* theP, _AVLListXL* varCache)
{
    _String fullName (aName);
    
    theParent = theP;


    if (aName.sLength) {
        long f = aName.Find('.');

        while (theP) {
            if (f!=-1) {
                f = aName.Find('.',f+1,-1);
            } else {
                break;
            }

            theP = theP->theParent;
        }

        if (theP) {
            fullName = (*(theP->theName))&'.'&fullName;
        }

        theName = (_String*)(fullName.makeDynamic());
        InsertVar (this);
    } else {
        fullName = *theName;
    }
    
    SetModel (FindModelName(theTmplt), varCache);
}

//__________________________________________________________________________________

void _VariableContainer::ScanAndAttachVariables (void)
{
    _Variable* curVar;
    _SimpleList travcache;

    long f = variableNames.Find (theName,travcache);
    if (f<0) {
        return;
    }

    _String theNameAndADot (*theName);
    theNameAndADot = theNameAndADot&'.';

    for (f = variableNames.Next (f, travcache); f>=0; f = variableNames.Next (f, travcache)) {
        curVar = FetchVar (f);

        if (curVar->theName->startswith(theNameAndADot)) {
            if (!curVar->IsContainer()) {
                long   vix = variableNames.GetXtra (f);

                if (curVar->IsIndependent()) {
                    if ( ((!iVariables)||iVariables->FindStepping(vix,2)==-1) && ((!dVariables)||dVariables->FindStepping(vix,2)==-1)) {
                        if (!iVariables) {
                            checkPointer (iVariables = new _SimpleList);
                        }
                        (*iVariables)<<vix;
                        (*iVariables)<<-1;
                    }
                } else {
                    if ( ((!iVariables)||iVariables->FindStepping(vix,2)==-1) && ((!dVariables)||dVariables->FindStepping(vix,2)==-1)) {
                        if (!dVariables) {
                            checkPointer (dVariables = new _SimpleList);
                        }
                        (*dVariables)<<vix;
                        (*dVariables)<<-1;
                    }
                }
            }
        } else {
            break;
        }
    }



}
//__________________________________________________________________________________

_VariableContainer::~_VariableContainer(void)
{
    if (iVariables) {
        delete iVariables;
    }
    if (dVariables) {
        delete dVariables;
    }
    if (gVariables) {
        delete gVariables;
    }
}

//__________________________________________________________________________________

bool _VariableContainer::HasChanged (void)
{
    long i;
    if (iVariables)
        for (i = 0; i<iVariables->lLength; i+=2)
            if (LocateVar (iVariables->lData[i])->HasChanged()) {
                return true;
            }

    if (gVariables)
        for (i = 0; i<gVariables->lLength; i++)
            if (LocateVar (gVariables->lData[i])->HasChanged()) {
                return true;
            }

    if (dVariables)
        for (i = 0; i<dVariables->lLength; i+=2)
            if (LocateVar (dVariables->lData[i])->HasChanged()) {
                return true;
            }

    return false;
}

//__________________________________________________________________________________

_Variable* _VariableContainer::GetIthIndependent (long index)
{
    if (iVariables && (index*=2)<iVariables->lLength) {
        return LocateVar (iVariables->lData[index]);
    } else {
        return nil;
    }
}

//__________________________________________________________________________________

_Variable* _VariableContainer::GetIthDependent (long index)
{
    if (dVariables && (index*=2)<dVariables->lLength) {
        return LocateVar (dVariables->lData[index]);
    } else {
        return nil;
    }
}

//__________________________________________________________________________________

_Variable* _VariableContainer::GetIthParameter (long index)
{
    if (iVariables) {
        if ( (index*=2) <iVariables->lLength) {
            return LocateVar (iVariables->lData[index]);
        } else {
            if (dVariables) {
                index-=iVariables->lLength;
                if (index<dVariables->lLength) {
                    return LocateVar (dVariables->lData[index]);
                }
            }
        }
    } else {
        if (dVariables && (index*=2) <dVariables->lLength) {
            return LocateVar (dVariables->lData[index]);
        }
    }
    return nil;
}

//__________________________________________________________________________________

bool _VariableContainer::NeedToExponentiate (bool ignoreCats)
{
    if (HY_VC_NO_CHECK&varFlags) {
        return false;
    }

    if (iVariables)
        for (long i = 0; i<iVariables->lLength && iVariables->lData[i+1] >= 0; i+=2) {
            if (LocateVar (iVariables->lData[i])->HasChanged(ignoreCats)) {
                return true;
            }
        }

    if (gVariables)
        for (long i = 0; i<gVariables->lLength; i++)
            if (LocateVar (gVariables->lData[i])->HasChanged(ignoreCats)) {
                return true;
            }
    if (dVariables)
        for (long i = 0; i<dVariables->lLength && dVariables->lData[i+1] >= 0; i+=2)
            if (LocateVar (dVariables->lData[i])->HasChanged(ignoreCats)) {
                return true;
            }

    return false;
}

//__________________________________________________________________________________
void      _VariableContainer::SortVars(void)
{
    // sort independents 1st
    // use dumb bubble sort
    bool        done = false;
    long        t,
                index;

    _String     *s1,
                *s2;

    if (iVariables && iVariables->lLength>2) {
        while (!done) {
            done = true;
            s1 = LocateVar(iVariables->lData[0])->GetName();
            for (index = 2; index<iVariables->lLength; index+=2) {
                s2 = LocateVar(iVariables->lData[index])->GetName();
                if (s2->Less (s1)) {
                    done = false;
                    t = iVariables->lData[index];
                    iVariables->lData    [index] = iVariables->lData[index-2];
                    iVariables->lData    [index-2] = t;

                    t = iVariables->lData[index+1];
                    iVariables->lData    [index+1] = iVariables->lData[index-1];
                    iVariables->lData    [index-1] = t;
                }

            }
        }
    }
    if (dVariables && dVariables->lLength>2) {
        done = false;
        while (!done) {
            done = true;
            s1 = LocateVar(dVariables->lData[0])->GetName();
            for (index = 2; index<dVariables->lLength; index+=2) {
                s2 = LocateVar(dVariables->lData[index])->GetName();
                if (s2->Less (s1)) {
                    done = false;
                    t = dVariables->lData[index];
                    dVariables->lData    [index] = dVariables->lData[index-2];
                    dVariables->lData    [index-2] = t;

                    t = dVariables->lData[index+1];
                    dVariables->lData    [index+1] = dVariables->lData[index-1];
                    dVariables->lData    [index-1] = t;
                }

            }
        }
    }
}
//__________________________________________________________________________________
bool      _VariableContainer::RemoveDependance (long varIndex)
{
    if (dVariables) {
        long f = dVariables->FindStepping(varIndex,2);

        if (f!=-1) {

            //printf ("Moving dep->ind for %s from %s\n", LocateVar (varIndex)->GetName()->sData,
            //      GetName()->sData);


            /*if (dVariables->lData[f+1]>=0)
            {
              _Variable* checkVar = LocateVar(dVariables->lData[f+1]);
               printf ("Local variable %s\n", checkVar->GetName()->sData);
              //if (!checkVar->IsIndependent())
                //  return false;
            }*/

            _String* thisName = LocateVar (dVariables->lData[f])->GetName();

            long insPos = 0;

            if (!iVariables) {
                checkPointer (iVariables = new _SimpleList);
            }

            while (insPos<iVariables->lLength && (thisName->Greater (LocateVar (iVariables->lData[insPos])->GetName()))) {
                insPos+=2;
            }


            iVariables->InsertElement ((BaseRef)varIndex, insPos, false, false);
            iVariables->InsertElement ((BaseRef)dVariables->lData[f+1], insPos+1, false, false);

            if (dVariables->lLength>2) {
                dVariables->Delete(f);
                dVariables->Delete(f);
                dVariables->TrimMemory();
            } else {
                delete dVariables;
                dVariables = nil;
            }
        }
    }
    return true;
}

//__________________________________________________________________________________
long      _VariableContainer::CheckAndAddUserExpression (_String& pName, long startWith)
{
    _String tryName, tryName2;
    tryName = (*theName)&'.'&pName;
    tryName2 = tryName;
    long    k = startWith>2?startWith:2;
    if (startWith>=2) {
        tryName2 = tryName&startWith;
    }

    while (LocateVarByName(tryName2)>=0) {
        tryName2 = tryName&k;
        k++;
    }

    if (startWith<0) {
        return k>2?k-1:0;
    }

    if (startWith<2) {
        if (k>2) {
            pName = pName&_String(k-1);
        }
    } else {
        if (k>startWith) {
            pName = pName & _String (k-1);
        } else {
            pName = pName & _String (startWith);
        }
    }

    _Variable newVar (tryName2);
    k =  newVar.GetAVariable();

    if (!dVariables) {
        checkPointer (dVariables = new _SimpleList);
    }
    (*dVariables) << k;
    (*dVariables) << -1;
    return k;
}

//__________________________________________________________________________________
void      _VariableContainer::CopyMatrixParameters (_VariableContainer* source)
{
    if (iVariables && source->iVariables)
        for (long i=0; i<iVariables->lLength && i< source->iVariables->lLength; i+=2) {
            LocateVar (iVariables->lData[i])->SetValue(LocateVar (source->iVariables->lData[i])->Compute());
        }

    _PMathObj srcVal = source->Compute();
    SetValue (srcVal);
}

//__________________________________________________________________________________
void      _VariableContainer::KillUserExpression (long varID)
{
    if (dVariables) {
        long f = dVariables->FindStepping(varID,2);
        if (f>=0) {
            DeleteVariable (*LocateVar(varID)->GetName(),true);
            if (dVariables->lLength > 2) {
                dVariables->Delete (f);
                dVariables->Delete (f);
                dVariables->TrimMemory ();
            } else {
                delete dVariables;
                dVariables = nil;
            }
        }
    }
}

//__________________________________________________________________________________
long      _VariableContainer::SetDependance (long varIndex)
{
    if (iVariables) {
        long f;

        if (varIndex>=0) {
            f = iVariables->FindStepping(varIndex,2);
            if (f<0) {
                return -1;
            }
        } else {
            f = -varIndex-1;
            varIndex = iVariables->lData[f];
        }


        //printf ("Moving ind->dep for %s from %s\n", LocateVar (varIndex)->GetName()->sData,
        //      GetName()->sData);

        if (iVariables->lData[f+1]>=0) {
            //printf ("Local variable %s\n", LocateVar (iVariables->lData[f+1])->GetName()->sData);
            if (!LocateVar(iVariables->lData[f+1])->IsIndependent()) {
                return -2;
            }
        }

        _String* thisName = LocateVar (iVariables->lData[f])->GetName();

        long    insPos = 0;

        if (!dVariables) {
            checkPointer (dVariables = new _SimpleList);
        }

        while (insPos<dVariables->lLength) {
            _Variable *dVar = LocateVar (dVariables->lData[insPos]);
            if (!dVar) {
                FlagError ("Internal error in SetDependance()");
                return -1;
            }
            if (!thisName->Greater (dVar->GetName())) {
                break;
            }
            insPos+=2;
        }

        dVariables->InsertElement ((BaseRef)varIndex, insPos, false, false);
        dVariables->InsertElement ((BaseRef)iVariables->lData[f+1], insPos+1, false, false);

        if (iVariables->lLength > 2) {
            iVariables->Delete(f);
            iVariables->Delete(f);
            iVariables->TrimMemory();
        } else {
            delete iVariables;
            iVariables = nil;
        }

        return varIndex;
    }
    return -1;
}

//__________________________________________________________________________________
bool      _VariableContainer::SetMDependance (_SimpleList& mDep)
{
    if (iVariables)
        if (mDep.lLength*2 > iVariables->lLength)
            for (long k=iVariables->lLength-2; k>=0; k-=2) {
                long f = mDep.BinaryFind (iVariables->lData[k]);
                if (f>=0) {
                    SetDependance (-k-1);
                }
            }
        else
            for (long k=0; iVariables && k<mDep.lLength; k++) {
                SetDependance (mDep.lData[k]);
            }

    return true;
}


//__________________________________________________________________________________
void      _VariableContainer::Clear(void)
{
    theModel = -1;
    if (iVariables) {
        delete iVariables;
        iVariables = nil;
    }
    if (dVariables) {
        delete dVariables;
        dVariables = nil;
    }
    if (gVariables) {
        delete gVariables;
        gVariables = nil;
    }
}

//__________________________________________________________________________________
long      _VariableContainer::CountAll(void)
{
    return (iVariables?iVariables->lLength/2:0)+(dVariables?dVariables->lLength/2:0);
}

//__________________________________________________________________________________
long      _VariableContainer::CountIndependents(void)
{
    return iVariables?iVariables->lLength/2:0;
}

//__________________________________________________________________________________
bool      _VariableContainer::HasLocals  (void)
{
    return (iVariables && iVariables->lLength)||(dVariables&&dVariables->lLength);
}

//__________________________________________________________________________________
bool      _VariableContainer::IsModelVar  (long i)
{
    return dVariables->lData[2*i+1]>=0;
}

//__________________________________________________________________________________

_String*    _VariableContainer::GetSaveableListOfUserParameters (void)
{
    _String * result = new _String (64L, true);
    checkPointer (result);

    if (dVariables)
        for (long i=0; i<dVariables->lLength; i+=2)
            if (dVariables->lData[i+1]<0) {
                _Variable * userParm  = (_Variable*) LocateVar (dVariables->lData[i]);
                _String   * varString = (_String*)userParm->GetFormulaString();
                *result << userParm->GetName();
                *result << ':';
                *result << '=';
                *result << varString;
                DeleteObject (varString);
                *result << ';';
                *result << '\n';
            }

    result->Finalize();
    return result;
}

//__________________________________________________________________________________
void      _VariableContainer::ClearConstraints(void)
{
    while (dVariables) {
        LocateVar(dVariables->lData[0])->ClearConstraints();
    }
}

//__________________________________________________________________________________

void  _VariableContainer::CompileListOfDependents (_SimpleList& rec)
{
    if (iVariables)
        for (long i=0; i<iVariables->lLength; i+=2) {
            LocateVar(iVariables->lData[i])->CompileListOfDependents (rec);
        }

    if (gVariables)
        for (long i=0; i<gVariables->lLength; i++) {
            LocateVar(gVariables->lData[i])->CompileListOfDependents (rec);
        }

    if (dVariables) {
        for (long i=0; i<dVariables->lLength; i+=2) {
            LocateVar(dVariables->lData[i])->CompileListOfDependents (rec);
        }

        {
            for (long i=0; i<dVariables->lLength; i+=2) {
                long f = rec.Find (dVariables->lData[i]);
                if (f>=0) {
                    rec.Delete (f);
                }
            }
        }
    }
}


//__________________________________________________________________________________

void _VariableContainer::MarkDone (void)
{
    if (iVariables)
        for (long i = 0; i<iVariables->lLength && iVariables->lData[i+1] >= 0; i+=2) {
            LocateVar (iVariables->lData[i])->MarkDone();
        }
    if (gVariables)
        for (long i = 0; i<gVariables->lLength; i++) {
            LocateVar (gVariables->lData[i])->MarkDone();
        }
}

//__________________________________________________________________________________

void _VariableContainer::MatchParametersToList (_List& suffixes, bool doAll, bool indOnly)
{
    if (doAll) {
        for (long i=suffixes.lLength-1; i>=0; i--) {
            long j;
            if (!indOnly) {
                if (dVariables) {
                    for (j=0; j<dVariables->lLength; j+=2)
                        if (LocateVar(dVariables->lData[j])->GetName()->endswith (*(_String*)suffixes.lData[i])) {
                            break;
                        }

                    if (j<dVariables->lLength) {
                        continue;
                    }
                }
            }
            if (iVariables) {
                for (j=0; j<iVariables->lLength; j+=2) {
                    if (LocateVar(iVariables->lData[j])->GetName()->endswith (*(_String*)suffixes.lData[i])) {
                        break;
                    }
                }
                if (j==iVariables->lLength) {
                    suffixes.Delete (i);
                }
            } else {
                suffixes.Delete (i);
            }
        }
    } else {
        for (long i=suffixes.lLength-1; i>=0; i--) {
            long j;
            if (dVariables) {
                for (j=0; j<dVariables->lLength; j+=2) {
                    if (dVariables->lData[j+1]<0) {
                        if (LocateVar(dVariables->lData[j])->GetName()->endswith (*(_String*)suffixes.lData[i])) {
                            break;
                        }
                    }
                }
                if (j==dVariables->lLength) {
                    suffixes.Delete (i);
                }
            } else {
                suffixes.Delete(i);
            }
        }
    }
}

//__________________________________________________________________________________

bool _VariableContainer::IsConstant (void)
{
    if (iVariables) {
        return false;
    }

    if (dVariables)
        for (long i = 0; i<dVariables->lLength; i+=2)
            if (!LocateVar(dVariables->lData[i])->IsConstant()) {
                return false;
            }

    if (gVariables)
        for (long i = 0; i<gVariables->lLength; i++)
            if (!LocateVar(gVariables->lData[i])->IsConstant()) {
                return false;
            }

    return true;
}

//__________________________________________________________________________________

void _VariableContainer::ScanForVariables (_AVLList& l,_AVLList& l2)
{
    if (iVariables)
        for (long i = 0; i<iVariables->lLength; i+=2) {
            l.Insert ((BaseRef)iVariables->lData[i]);
        }
    if (dVariables)
        for (long i = 0; i<dVariables->lLength; i+=2) {
            l2.Insert ((BaseRef)dVariables->lData[i]);
            _SimpleList temp;
            {
                _AVLList  ta (&temp);
                LocateVar (dVariables->lData[i])->ScanForVariables(ta, true);
                ta.ReorderList();
            }
            // see if any of them are global
            for (long j=0; j<temp.lLength; j++) {
                long p = temp.lData[j];
                _Variable * v = LocateVar(p);
                if (!v->IsGlobal() && v->IsIndependent()) {
                    l.Insert ((BaseRef)p);
                }
            }
        }
}

//__________________________________________________________________________________

void _VariableContainer::ScanForDVariables (_AVLList& l,_AVLList&)
{
    if (dVariables)
        for (long i = 0; i<dVariables->lLength; i+=2) {
            l.Insert ((BaseRef)dVariables->lData[i]);
        }
}

//__________________________________________________________________________________

void _VariableContainer::GetListOfModelParameters (_List& rec)
{
    if (iVariables)
        for (long i = 1; i<iVariables->lLength; i+=2) {
            long p = iVariables->lData[i];
            if (p>=0) {
                rec << LocateVar(p)->GetName();
            }
        }
}

//__________________________________________________________________________________

void _VariableContainer::ScanForGVariables (_AVLList& l,_AVLList& l2)
{
    if (gVariables)
        for (long i = 0; i<gVariables->lLength; i++) {
            long p = gVariables->lData[i];
            _Variable *v = LocateVar (p);
            if (v->IsIndependent()) {
                l.Insert ((BaseRef)p);
            } else {
                l2.Insert ((BaseRef)p);
            }
        }
    // additionally, check to see if there is any implicit dependence on the global variables yet unseen
    if (dVariables)
        for (long i = 0; i<dVariables->lLength; i+=2) {
            _SimpleList temp;
            {
                _AVLList  al (&temp);
                _Variable *v = LocateVar (dVariables->lData[i]);
                v->ScanForVariables(al, true);
                al.ReorderList();
            }
            // see if any of them are global
            for (long j=0; j<temp.lLength; j++) {
                long p = temp.lData[j];
                _Variable * v = LocateVar(p);
                if (v->IsGlobal()) { // good sign!
                    if (v->IsIndependent()) {
                        l.Insert ((BaseRef)p);
                    } else {
                        l2.Insert ((BaseRef)p);
                    }

                }
            }
        }
}
