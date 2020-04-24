#ifdef __cplusplus

void	swab( const void* src1, const void* src2, int isize)
{
	register char*	ptr1;
	register char*	ptr2;
	register int		ic1;
	
	ptr1 = (char*)src1;
	ptr2 = (char*)src2;
	for ( ic1=0 ; ic1<isize ; ic1+=2){
		register char t = ptr1[ic1+0];
		ptr2[ic1+0] = ptr1[ic1+1];
		ptr2[ic1+1] = t;
	}
}

#endif
