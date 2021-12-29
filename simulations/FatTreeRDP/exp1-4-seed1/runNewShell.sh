echo " Running FatTree Traffic .... "
find . -name "*.vec" -type f -delete|find . -name  "*.sca"  -type f -delete| find . -name "*.vci" -type f -delete | find . -name "*.csv" -type f -delete
opp_run_release -u Cmdenv -m -n ../..:../../../src:../../../../inet4/src:../../../../inet4/examples:../../../../inet4/tutorials:../../../../inet4/showcases -l ../../../src/ndp ../exp1-4-seed1.ini -c General -r "\$FatTreeSize==$(printf '%d' $1)  &&  \$numShortFlows==$(printf '%d' $2)"

echo "Throughput"
scavetool export -T s -f "module(**.app[*]) AND ("mohThroughputNDP:last")"   -F CSV-S -o instThroughput.csv *.sca
echo "   "
echo "Throughput 2 "
scavetool export -T s -f "module(**.app[*]) AND ("instThroughputNDP:mean")"   -F CSV-S -o instThroughput2.csv *.sca
echo "   "
scavetool export -T s -f "module(**.app[*]) AND ("multicastGroupIdSignal:last")"   -F CSV-S -o multicastgroups.csv *.sca
scavetool export -T s -f "module(**.app[*]) AND ("multisourceGroupIdSignal:last")"   -F CSV-S -o multisourcegroups.csv *.sca
cat multicastgroups.csv | cut -d, -f7  | sed "1 d" > MatMulticastgroups.csv
cat multisourcegroups.csv | cut -d, -f7  | sed "1 d" > MatMultisourcegroups.csv
echo "FCT"
scavetool export -T s -f "module(**.app[*]) AND ("fctRecordv3:last")"   -F CSV-S -o fct.csv *.sca
echo "   "
echo "header"
scavetool export -T s -f "module(**.app[*]) AND ("numRcvTrimmedHeaderSigNdp:last")"   -F CSV-S -o numRcvHeader.csv *.sca
echo "   "
 cat numRcvHeader.csv | cut -d, -f7  | sed "1 d" > MatNumRcvHeader.csv
echo "num trimmed packets"
scavetool export -T s -f 'module(*.CoreRouter[*].*) AND  name("numTrimmedPkt:last") '   -F CSV-S -o numTrimmedPktCore.csv *.sca
scavetool export -T s -f 'module(*.aggRouters[*].*) AND  name("numTrimmedPkt:last") '   -F CSV-S -o numTrimmedPktAgg.csv *.sca
scavetool export -T s -f 'module(*.edgeSwitch.*) AND  name("numTrimmedPkt:last") '   -F CSV-S -o numTrimmedPktEdge.csv *.sca
echo "CoreRouter-packetPushed:count"
scavetool export -T s -f 'module(*.CoreRouter[*].*) AND name("packetPushed:count") '   -F CSV-S -o coreRouterRcvdPkt.csv *.sca
echo "CoreRouter-packetDropped:count"
scavetool export -T s -f "module(*.CoreRouter[*].*) AND name(packetDropped:count)"   -F CSV-S -o coreRouterDropPkt.csv *.sca
echo "   "
echo "aggRouters-packetPushed:count"
scavetool export -T s -f 'module(*.aggRouters[*].*) AND  name("packetPushed:count") '   -F CSV-S -o aggRouterRcvdPkt.csv *.sca
echo "   "
echo "aggRouters-packetDropped:count"
scavetool export -T s -f "module(*.aggRouters[*].*) AND name(packetDropped:count)"   -F CSV-S -o aggRouterDropPkt.csv *.sca
echo "   "
echo "edgeSwitch-packetPushed:count"
scavetool export -T s -f 'module(*.edgeSwitch.*) AND  name("packetPushed:count") '   -F CSV-S -o edgeRouterRcvdPkt.csv *.sca
echo "   "
echo "edgeSwitch-packetDropped:count"
scavetool export -T s -f "module(*.edgeSwitch.*) AND name(packetDropped:count)"   -F CSV-S -o edgeRouterDropPkt.csv *.sca
echo "   "
echo "servers-packetPushed:sum(packetBytes)"
scavetool export -T s -f 'module(FatTreeNdp.Pod[*].racks[*].servers[*].ppp[*].queue) AND  name("packetReceived:sum(packetBytes)") '   -F CSV-S -o serversRcvdPktBytes.csv *.sca
echo "servers-packetDropped:sum(packetBytes)"
scavetool export -T s -f 'module(FatTreeNdp.Pod[*].racks[*].servers[*].ppp[*].queue) AND  name("packetDropped:sum(packetBytes)") '   -F CSV-S -o serversDropPktBytes.csv *.sca
echo "servers-packetReceived:count"
scavetool export -T s -f 'module(FatTreeNdp.Pod[*].racks[*].servers[*].ppp[*].queue) AND  name("packetReceived:count") '   -F CSV-S -o serversRcvdPkt.csv *.sca
echo "servers-packetDropped:count"
scavetool export -T s -f 'module(FatTreeNdp.Pod[*].racks[*].servers[*].ppp[*].queue) AND  name("packetDropped:count") '   -F CSV-S -o serversDropPkt.csv *.sca

echo "   "
echo "config."
scavetool export -T s -f " name(simTimeTotal=)  OR name(numShortFlows=)  OR name(flowSize=) OR name(percentLongFlowNodes=) OR name(arrivalRate=) OR name(randTM)  OR name(permTM)  OR name(wallClockTime=) OR name(IW=) OR name(ndpSwitchQueueLength=) OR name(perPacketEcmp=) OR name(perFlowEcmp=) OR name(oneToOne=) OR name(oneToMany=) OR name(manyToOne=)  OR name(seedValue=) OR name(kValue=)   OR name(numReplica=) OR name(numRunningMulticastGroups=) OR name(numRunningMultiSourcingGroups=)  OR name(isWebSearchWorkLoad=)  "   -F CSV-S -o config.csv *.sca
echo "   "

echo "FatTreeNdp.centralSchedulerNdp "
scavetool export -T v -f 'module(FatTreeNdp.centralSchedulerNdp) AND name(numTcpSessionAppsVec)'  -F CSV-R -o numTcpSessionApps.csv  *.vec
scavetool export -T v -f 'module(FatTreeNdp.centralSchedulerNdp) AND name(numTcpSinkAppsVec)'  -F CSV-R -o numTcpSinkApps.csv  *.vec
scavetool export -T v -f 'module(FatTreeNdp.centralSchedulerNdp) AND name(nodes)'  -F CSV-R -o nodes.csv  *.vec
scavetool export -T v -f 'module(FatTreeNdp.centralSchedulerNdp) AND name(matSrc)'  -F CSV-R -o matrixSrc.csv  *.vec
scavetool export -T v -f 'module(FatTreeNdp.centralSchedulerNdp) AND name(matDest)'  -F CSV-R -o matrixDest.csv  *.vec
scavetool export -T v -f 'module(FatTreeNdp.centralSchedulerNdp) AND name(permMapLongFlowsVec)'  -F CSV-R -o longFlowsNodes.csv  *.vec
scavetool export -T v -f 'module(FatTreeNdp.centralSchedulerNdp) AND name(permMapShortFlowsVec)'  -F CSV-R -o shortFlowsNodes.csv  *.vec
scavetool export -T v -f 'module(FatTreeNdp.centralSchedulerNdp) AND name(randMapShortFlowsVec)'  -F CSV-R -o randShortFlowsNodes.csv  *.vec
scavetool export -T v -f 'module(FatTreeNdp.centralSchedulerNdp) AND name(permMapShortFlowsVector)'  -F CSV-R -o permMapShortFlowsVector.csv  *.vec



cat fct.csv | cut -d, -f7  | sed "1 d" > MatFct.csv
scavetool export -T s -f "  module(**.app[*]) AND ("fctRecordv3:last")     "  -F CSV-S -o a-fct.csv *.sca
cat a-fct.csv | cut -d, -f7  | sed "1 d" > aaMatFct.csv
scavetool export -T s -f 'module(*.app[*]) AND name("packetReceived:sum(packetBytes)")    '   -F CSV-S -o a-flowsizes.csv *.sca
cat a-flowsizes.csv | cut -d, -f7  | sed "1 d" > aaMatFlowSizes.csv
cat instThroughput.csv | cut -d, -f7  | sed "1 d" > MatInstThroughput.csv
cat instThroughput2.csv | cut -d, -f7  | sed "1 d" > MatInstThroughput2.csv
cat coreRouterRcvdPkt.csv | cut -d, -f7  | sed "1 d" > MatCoreRouterRcvdPkt.csv
cat coreRouterDropPkt.csv | cut -d, -f7  | sed "1 d" > MatCoreRouterDropPkt.csv

cat aggRouterRcvdPkt.csv | cut -d, -f7  | sed "1 d" > MatAggRouterRcvdPkt.csv
cat aggRouterDropPkt.csv | cut -d, -f7  | sed "1 d" > MatAggRouterDropPkt.csv

cat aggRouterDropPkt.csv | cut -d, -f7  | sed "1 d" > MatAggRouterDropPkt.csv
cat edgeRouterRcvdPkt.csv | cut -d, -f7  | sed "1 d" > MatEdgeRouterRcvdPkt.csv
cat edgeRouterDropPkt.csv | cut -d, -f7  | sed "1 d" > MatEdgeRouterDropPkt.csv
cat serversRcvdPktBytes.csv | cut -d, -f7  | sed "1 d" > MatServersRcvdPktBytes.csv

cat serversRcvdPkt.csv | cut -d, -f7  | sed "1 d" > MatServersRcvdPkt.csv

cat serversDropPkt.csv | cut -d, -f7  | sed "1 d" > MatServersDropPkt.csv
cat serversDropPktBytes.csv | cut -d, -f7  | sed "1 d" > MatServersDropPktBytes.csv

cat config.csv | cut -d, -f7  | sed "1 d" > MatConfig.csv
cat numTcpSessionApps.csv | cut -d, -f9  | sed "1 d" > MatNumTcpSessionApps.csv
cat numTcpSinkApps.csv | cut -d, -f9  | sed "1 d" > MatNumTcpSinkApps.csv
cat nodes.csv | cut -d, -f9  | sed "1 d" > MatNodes.csv

cat matrixSrc.csv | cut -d, -f9  | sed "1 d" > MatMatrixSrc.csv
cat matrixDest.csv | cut -d, -f9  | sed "1 d" > MatMatrixDest.csv

cat longFlowsNodes.csv | cut -d, -f9  | sed "1 d" > MatLongFlowsNodes.csv
cat shortFlowsNodes.csv | cut -d, -f9  | sed "1 d" > MatShortFlowsNodes.csv

cat randShortFlowsNodes.csv | cut -d, -f9  | sed "1 d" > MatRandShortFlowsNodes.csv
cat randShortFlowsNodes.csv | cut -d, -f9  | sed "1 d" > MatRandShortFlowsNodes.csv

cat permMapShortFlowsVector.csv | cut -d, -f9  | sed "1 d" > MatPermMapShortFlowsVector.csv
cat numTrimmedPktCore.csv | cut -d, -f7  | sed "1 d" > MatNumTrimmedPktCore.csv
cat numTrimmedPktAgg.csv | cut -d, -f7  | sed "1 d" > MatNumTrimmedPktAgg.csv
cat numTrimmedPktEdge.csv | cut -d, -f7  | sed "1 d" > MatNumTrimmedPktEdge.csv


if [ "$3" = "-p" ]  #p: plotting
then
$HOME/MATLAB/R2021b/bin/matlab -nodesktop -nosplash    -r "cd('matlabScriptsNew/'); plotResults"
elif [ "$3" = "-ph" ] #p: plotting on HPC
then
$HOME/MATLAB/R2021b/bin/matlab -nodesktop -nosplash    -r "cd('matlabScriptsNew/'); plotResults"
else
echo "no plotting"
fi


