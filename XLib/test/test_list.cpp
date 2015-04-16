#include <xlist.h>

typedef ZQ::XListT<void* , void* > CPtrList;
typedef ZQ::XListT<int, int> CIntList;

int main()
{
	/*
	CPtrList list;
	POSITION pos = list.AddTail(NULL);
	list.RemoveAt(pos);
	list.AddHead(NULL);
	printf("%d\n", list.GetCount());
	*/

	CIntList list;
	for (int i = 0; i < 10; i ++)
		list.AddTail(i);

	int& n = list.GetHead();

	CIntList::POSITION pos1 = list.GetHeadPosition();
	list.SetAt(pos1, 186);

	CIntList::POSITION pos = list.GetHeadPosition();
	//list.InsertBefore(pos, 99);
	//list.SetAt(pos, 100);
	//printf("%d\n", list.GetTail());
	pos = list.InsertBefore(pos, 100);
	list.SetAt(pos, 77);

	pos = list.FindIndex(10);
	printf("%d\n", list.GetNext(pos));

	
	pos = list.GetHeadPosition();
	while (pos != NULL) {
		int n = list.GetNext(pos);
		printf("%d\n", n);
	}

	return 0;
}
