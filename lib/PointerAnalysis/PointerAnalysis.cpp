// Author: Jingyue

// PointerAnalysis is an abstract interface for any pointer analysis. 
// It has the interface <getPointees> which takes a pointer and returns
// the set of all pointees of this pointer. 
// 
// PointerAnalysis is not a pass itself, which is similar to AliasAnalysis.
// Any inherited class of PointerAnalysis should implement their own point-to
// analysis if necessary. 
//
// PointerAnalysis is a AnalysisGroup. The default instance of this group
// is BasicPointerAnalysis. 
