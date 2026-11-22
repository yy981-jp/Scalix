#include <physics/collider.h>


// update
#define func(Type) WorldCollider WSH::UpdateVisitor::operator()(const CLSH::Type& col) const
func(Sphere) {
	
}
func(Capsule) {

}
#undef func

// collide
#define func(AType,BType) bool WSH::CollideVisitor::operator()(const AType& a, const BType& b) const
#define func_m(AType,BType) func(BType,AType) { return this->operator()(b,a); } func(AType,BType)
func(Sphere,Sphere) {
	// TODO
}
func_m(Sphere,Capsule) {
	// TODO
}
func(Capsule,Capsule) {
	// TODO
}
#undef func
#undef func_m
