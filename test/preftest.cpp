#include "../XMLPref.h"
#include "../CombString.h"
void main()
{
	ZQ::common::ComInitializer init;

	ZQ::common::XMLPrefDoc doc(init);
	if (!doc.open("McastFwd.xml"))
		return;

	ZQ::common::IPreference* root = doc.root();
	char buf[2048];
	root->name(buf);
	for(ZQ::common::IPreference* deft= root->firstChild(); deft; deft=root->nextChild())
	{
        deft->name(buf);
		deft->free();
	}

	root->free();
	
	///
	for(ZQ::common::PrefGuard childGd(rootGd.pref()->firstChild()); childGd.valid(); childGd.pref(rootGd.pref()->nextChild()))
	{
		childGd.pref()->name(buf);
	}
}