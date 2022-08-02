OUTDIR=build
KWS3=${OUTDIR}/kws3

all: ${KWS3}

run: all
	${KWS3}

stop:
	echo 'shutdown; quit' | nc localhost 9003

clean:
	rm -rf "${OUTDIR}"

${OUTDIR}:
	mkdir -p "${OUTDIR}"

${OUTDIR}/util.o:                      \
		util.h                         \
		util.cc
	g++ -o ${OUTDIR}/util.o -c util.cc

${OUTDIR}/connection.o:                \
		connection.h                   \
		connection.cc
	g++ -o ${OUTDIR}/connection.o -c connection.cc

${OUTDIR}/tcpListener.o:               \
		tcpListener.h                  \
		tcpListener.cc                 \
		${OUTDIR}/connection.o
	g++ -o ${OUTDIR}/tcpListener.o -c tcpListener.cc

${OUTDIR}/httpConnection.o:            \
		httpConnection.h               \
		httpConnection.cc              \
		shmemStat.h                    \
		${OUTDIR}/util.o               \
		${OUTDIR}/httpResponse.o       \
		${OUTDIR}/connection.o
	g++ -o ${OUTDIR}/httpConnection.o -c httpConnection.cc

${OUTDIR}/httpResponse.o:              \
		httpResponse.h                 \
		httpResponse.cc                \
		shmemStat.h                    \
		${OUTDIR}/connection.o
	g++ -o ${OUTDIR}/httpResponse.o -c httpResponse.cc

${OUTDIR}/pipeConnection.o:            \
		pipeConnection.h               \
		pipeConnection.cc              \
		${OUTDIR}/connection.o
	g++ -o ${OUTDIR}/pipeConnection.o -c pipeConnection.cc

${OUTDIR}/cmdConnection.o:             \
		cmdConnection.h                \
		cmdConnection.cc               \
		shmemStat.h                    \
		${OUTDIR}/httpConnection.o     \
		${OUTDIR}/pipeConnection.o     \
		${OUTDIR}/connection.o
	g++ -o ${OUTDIR}/cmdConnection.o -c cmdConnection.cc

${OUTDIR}/server.o:                    \
		server.h                       \
		server.cc                      \
		${OUTDIR}/tcpListener.o        \
		${OUTDIR}/httpConnection.o     \
		${OUTDIR}/cmdConnection.o      \
		${OUTDIR}/pipeConnection.o
	g++ -o ${OUTDIR}/server.o -c server.cc

${KWS3}: ${OUTDIR}                     \
		main.cc                        \
		${OUTDIR}/server.o
	g++ -o "${KWS3}" ${OUTDIR}/*.o main.cc
