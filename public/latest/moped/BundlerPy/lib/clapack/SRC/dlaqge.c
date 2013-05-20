#include "f2c.h"
#include "blaswrap.h"

/* Subroutine */ int dlaqge_(integer *m, integer *n, doublereal *a, integer *
	lda, doublereal *r__, doublereal *c__, doublereal *rowcnd, doublereal 
	*colcnd, doublereal *amax, char *equed)
{
    /* System generated locals */
    integer a_dim1, a_offset, i__1, i__2;

    /* Local variables */
    integer i__, j;
    doublereal cj, large, small;
    extern doublereal dlamch_(char *);


/*  -- LAPACK auxiliary routine (version 3.1) -- */
/*     Univ. of Tennessee, Univ. of California Berkeley and NAG Ltd.. */
/*     November 2006 */

/*     .. Scalar Arguments .. */
/*     .. */
/*     .. Array Arguments .. */
/*     .. */

/*  Purpose */
/*  ======= */

/*  DLAQGE equilibrates a general M by N matrix A using the row and */
/*  column scaling factors in the vectors R and C. */

/*  Arguments */
/*  ========= */

/*  M       (input) INTEGER */
/*          The number of rows of the matrix A.  M >= 0. */

/*  N       (input) INTEGER */
/*          The number of columns of the matrix A.  N >= 0. */

/*  A       (input/output) DOUBLE PRECISION array, dimension (LDA,N) */
/*          On entry, the M by N matrix A. */
/*          On exit, the equilibrated matrix.  See EQUED for the form of */
/*          the equilibrated matrix. */

/*  LDA     (input) INTEGER */
/*          The leading dimension of the array A.  LDA >= max(M,1). */

/*  R       (input) DOUBLE PRECISION array, dimension (M) */
/*          The row scale factors for A. */

/*  C       (input) DOUBLE PRECISION array, dimension (N) */
/*          The column scale factors for A. */

/*  ROWCND  (input) DOUBLE PRECISION */
/*          Ratio of the smallest R(i) to the largest R(i). */

/*  COLCND  (input) DOUBLE PRECISION */
/*          Ratio of the smallest C(i) to the largest C(i). */

/*  AMAX    (input) DOUBLE PRECISION */
/*          Absolute value of largest matrix entry. */

/*  EQUED   (output) CHARACTER*1 */
/*          Specifies the form of equilibration that was done. */
/*          = 'N':  No equilibration */
/*          = 'R':  Row equilibration, i.e., A has been premultiplied by */
/*                  diag(R). */
/*          = 'C':  Column equilibration, i.e., A has been postmultiplied */
/*                  by diag(C). */
/*          = 'B':  Both row and column equilibration, i.e., A has been */
/*                  replaced by diag(R) * A * diag(C). */

/*  Internal Parameters */
/*  =================== */

/*  THRESH is a threshold value used to decide if row or column scaling */
/*  should be done based on the ratio of the row or column scaling */
/*  factors.  If ROWCND < THRESH, row scaling is done, and if */
/*  COLCND < THRESH, column scaling is done. */

/*  LARGE and SMALL are threshold values used to decide if row scaling */
/*  should be done based on the absolute size of the largest matrix */
/*  element.  If AMAX > LARGE or AMAX < SMALL, row scaling is done. */

/*  ===================================================================== */

/*     .. Parameters .. */
/*     .. */
/*     .. Local Scalars .. */
/*     .. */
/*     .. External Functions .. */
/*     .. */
/*     .. Executable Statements .. */

/*     Quick return if possible */

    /* Parameter adjustments */
    a_dim1 = *lda;
    a_offset = 1 + a_dim1;
    a -= a_offset;
    --r__;
    --c__;

    /* Function Body */
    if (*m <= 0 || *n <= 0) {
	*(unsigned char *)equed = 'N';
	return 0;
    }

/*     Initialize LARGE and SMALL. */

    small = dlamch_("Safe minimum") / dlamch_("Precision");
    large = 1. / small;

    if (*rowcnd >= .1 && *amax >= small && *amax <= large) {

/*        No row scaling */

	if (*colcnd >= .1) {

/*           No column scaling */

	    *(unsigned char *)equed = 'N';
	} else {

/*           Column scaling */

	    i__1 = *n;
	    for (j = 1; j <= i__1; ++j) {
		cj = c__[j];
		i__2 = *m;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    a[i__ + j * a_dim1] = cj * a[i__ + j * a_dim1];
/* L10: */
		}
/* L20: */
	    }
	    *(unsigned char *)equed = 'C';
	}
    } else if (*colcnd >= .1) {

/*        Row scaling, no column scaling */

	i__1 = *n;
	for (j = 1; j <= i__1; ++j) {
	    i__2 = *m;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		a[i__ + j * a_dim1] = r__[i__] * a[i__ + j * a_dim1];
/* L30: */
	    }
/* L40: */
	}
	*(unsigned char *)equed = 'R';
    } else {

/*        Row and column scaling */

	i__1 = *n;
	for (j = 1; j <= i__1; ++j) {
	    cj = c__[j];
	    i__2 = *m;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		a[i__ + j * a_dim1] = cj * r__[i__] * a[i__ + j * a_dim1];
/* L50: */
	    }
/* L60: */
	}
	*(unsigned char *)equed = 'B';
    }

    return 0;

/*     End of DLAQGE */

} /* dlaqge_ */
