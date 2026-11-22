#include <physics/colliderDef.h>
#include <util/math.h>


// update
#define func(Type) WorldCollider WSH::UpdateVisitor::operator()(const CLSH::Type& col) const
func(Sphere) {
	WorldCollider wcl;
	WSH::Sphere wsh;

	wsh.center = col.offset * mtx;
	wsh.radius = col.radius * maxAbsComponent(mtx.scale());

	wcl.shape = wsh;
	return wcl;
}
func(Capsule) {
	WorldCollider wcl;
	WSH::Capsule wsh;

	wsh.head = col.offset * mtx;
	wsh.tail = col.tail * mtx;
	wsh.radius = col.radius * maxAbsComponent(mtx.scale());

	wcl.shape = wsh;
	return wcl;
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
