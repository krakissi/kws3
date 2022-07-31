OUTDIR=build
KWS3=${OUTDIR}/kws3

all: ${KWS3}

run: all
	${KWS3}

clean:
	rm -rf "${OUTDIR}"

${OUTDIR}:
	mkdir -p "${OUTDIR}"

${OUTDIR}/util.o: util.h util.cc
	g++ -o ${OUTDIR}/util.o -c util.cc

${OUTDIR}/httpConnection.o: httpConnection.h httpConnection.cc
	g++ -o ${OUTDIR}/httpConnection.o -c httpConnection.cc

${KWS3}: ${OUTDIR}                     \
		main.cc                        \
		${OUTDIR}/util.o               \
		${OUTDIR}/httpConnection.o
	g++ -o "${KWS3}" -s ${OUTDIR}/*.o main.cc
