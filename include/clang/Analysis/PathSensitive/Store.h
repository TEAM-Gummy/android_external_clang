//== Store.h - Interface for maps from Locations to Values ------*- C++ -*--==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defined the types Store and StoreManager.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_ANALYSIS_STORE_H
#define LLVM_CLANG_ANALYSIS_STORE_H

#include "clang/Analysis/PathSensitive/MemRegion.h"
#include "clang/Analysis/PathSensitive/SVals.h"
#include "clang/Analysis/PathSensitive/ValueManager.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include <iosfwd>

namespace clang {
  
typedef const void* Store;

class GRState;  
class GRStateManager;
class Stmt;
class Expr;
class ObjCIvarDecl;
class SubRegionMap;
  
class StoreManager {
protected:
  ValueManager &ValMgr;
  GRStateManager &StateMgr;

  /// MRMgr - Manages region objects associated with this StoreManager.
  MemRegionManager &MRMgr;

  StoreManager(GRStateManager &stateMgr);

protected:
  virtual const GRState *AddRegionView(const GRState *state,
                                        const MemRegion *view,
                                        const MemRegion *base) {
    return state;
  }
  
public:  
  virtual ~StoreManager() {}
  
  /// Return the value bound to specified location in a given state.
  /// \param[in] state The analysis state.
  /// \param[in] loc The symbolic memory location.
  /// \param[in] T An optional type that provides a hint indicating the 
  ///   expected type of the returned value.  This is used if the value is
  ///   lazily computed.
  /// \return The value bound to the location \c loc.
  virtual SVal Retrieve(const GRState *state, Loc loc,
                        QualType T = QualType()) = 0;

  /// Return a state with the specified value bound to the given location.
  /// \param[in] state The analysis state.
  /// \param[in] loc The symbolic memory location.
  /// \param[in] val The value to bind to location \c loc.
  /// \return A pointer to a GRState object that contains the same bindings as 
  ///   \c state with the addition of having the value specified by \c val bound
  ///   to the location given for \c loc.
  virtual const GRState *Bind(const GRState *state, Loc loc, SVal val) = 0;

  virtual Store Remove(Store St, Loc L) = 0;
  
  /// BindCompoundLiteral - Return the store that has the bindings currently
  ///  in 'store' plus the bindings for the CompoundLiteral.  'R' is the region
  ///  for the compound literal and 'BegInit' and 'EndInit' represent an
  ///  array of initializer values.
  virtual const GRState *BindCompoundLiteral(const GRState *state,
                                              const CompoundLiteralExpr* cl,
                                              SVal v) = 0;
  
  /// getInitialStore - Returns the initial "empty" store representing the
  ///  value bindings upon entry to an analyzed function.
  virtual Store getInitialStore() = 0;
  
  /// getRegionManager - Returns the internal RegionManager object that is
  ///  used to query and manipulate MemRegion objects.
  MemRegionManager& getRegionManager() { return MRMgr; }
  
  /// getSubRegionMap - Returns an opaque map object that clients can query
  ///  to get the subregions of a given MemRegion object.  It is the
  //   caller's responsibility to 'delete' the returned map.
  virtual SubRegionMap *getSubRegionMap(const GRState *state) = 0;

  virtual SVal getLValueVar(const GRState *state, const VarDecl *vd) = 0;

  virtual SVal getLValueString(const GRState *state,
                               const StringLiteral* sl) = 0;

  virtual SVal getLValueCompoundLiteral(const GRState *state,
                                        const CompoundLiteralExpr* cl) = 0;
  
  virtual SVal getLValueIvar(const GRState *state, const ObjCIvarDecl* decl,
                             SVal base) = 0;
  
  virtual SVal getLValueField(const GRState *state, SVal base,
                              const FieldDecl* D) = 0;
  
  virtual SVal getLValueElement(const GRState *state, QualType elementType,
                                SVal base, SVal offset) = 0;

  // FIXME: Make out-of-line.
  virtual SVal getSizeInElements(const GRState *state, const MemRegion *region){
    return UnknownVal();
  }

  /// ArrayToPointer - Used by GRExprEngine::VistCast to handle implicit
  ///  conversions between arrays and pointers.
  virtual SVal ArrayToPointer(Loc Array) = 0;
  
  class CastResult {
    const GRState *state;
    const MemRegion *region;
  public:
    const GRState *getState() const { return state; }
    const MemRegion* getRegion() const { return region; }
    CastResult(const GRState *s, const MemRegion* r = 0) : state(s), region(r){}
  };
  
  /// CastRegion - Used by GRExprEngine::VisitCast to handle casts from
  ///  a MemRegion* to a specific location type.  'R' is the region being
  ///  casted and 'CastToTy' the result type of the cast.
  virtual CastResult CastRegion(const GRState *state, const MemRegion *region,
                                QualType CastToTy);

  /// EvalBinOp - Perform pointer arithmetic.
  virtual SVal EvalBinOp(const GRState *state, BinaryOperator::Opcode Op,
                         Loc lhs, NonLoc rhs) {
    return UnknownVal();
  }
  
  /// getSelfRegion - Returns the region for the 'self' (Objective-C) or
  ///  'this' object (C++).  When used when analyzing a normal function this
  ///  method returns NULL.
  virtual const MemRegion* getSelfRegion(Store store) = 0;

  virtual Store RemoveDeadBindings(const GRState *state,
                                   Stmt* Loc, SymbolReaper& SymReaper,
                      llvm::SmallVectorImpl<const MemRegion*>& RegionRoots) = 0;

  virtual const GRState *BindDecl(const GRState *state, const VarDecl *vd, 
                                   SVal initVal) = 0;

  virtual const GRState *BindDeclWithNoInit(const GRState *state,
                                             const VarDecl *vd) = 0;

  // FIXME: Make out-of-line.
  virtual const GRState *setExtent(const GRState *state,
                                    const MemRegion *region, SVal extent) {
    return state;
  }

  // FIXME: Make out-of-line.
  virtual const GRState *setDefaultValue(const GRState *state,
                                          const MemRegion *region,
                                          SVal val) {
    return state;
  }

  virtual void print(Store store, std::ostream& Out,
                     const char* nl, const char *sep) = 0;
      
  class BindingsHandler {
  public:    
    virtual ~BindingsHandler();
    virtual bool HandleBinding(StoreManager& SMgr, Store store,
                               const MemRegion *region, SVal val) = 0;
  };
  
  /// iterBindings - Iterate over the bindings in the Store.
  virtual void iterBindings(Store store, BindingsHandler& f) = 0;  
};

// FIXME: Do we still need this?
/// SubRegionMap - An abstract interface that represents a queryable map
///  between MemRegion objects and their subregions.
class SubRegionMap {
public:
  virtual ~SubRegionMap() {}
  
  class Visitor {
  public:
    virtual ~Visitor() {};
    virtual bool Visit(const MemRegion* Parent, const MemRegion* SubRegion) = 0;
  };
  
  virtual bool iterSubRegions(const MemRegion *region, Visitor& V) const = 0;  
};

// FIXME: Do we need to pass GRStateManager anymore?
StoreManager *CreateBasicStoreManager(GRStateManager& StMgr);
StoreManager *CreateRegionStoreManager(GRStateManager& StMgr);
StoreManager *CreateFieldsOnlyRegionStoreManager(GRStateManager& StMgr);

} // end clang namespace

#endif
