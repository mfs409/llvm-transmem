import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

stm = np.genfromtxt('result/data/tpccbp_f1.txt', delimiter=',', usecols=np.arange(0,7))
nptm = np.genfromtxt('result/data/tpccbp_f1.txt', delimiter=',', usecols=np.arange(7,14))
cgl = np.genfromtxt('result/data/tpccbp_f1.txt', delimiter=',', usecols=14)
x = np.array([1,7,14,20,27,34,41,48])
plt.xlim(0,50)
plt.xticks(x,['1', '2', '4', '8', '16', '24', '32', '48'])

subj1=["cgl_mutex", "orec_eager", "orec_lazy", "orec_mixed", "norec", "ring_sw", "tlrw_eager", "pn_cgl_eager", "pn_orec_eager", "pn_orec_lazy", "pn_orec_mixed", "pn_norec", "pn_ring_sw", "pn_tlrw_eager", "pn_cgl_lazy"]

plt.plot(x,stm)
plt.title("TPCC (B+Tree)")
plt.plot(x,nptm, linestyle='dashed')
plt.plot(x,cgl, linestyle='dashed', color='dodgerblue')
plt.xlabel("Number of Threads")
plt.ylabel("Throughput (Ktps)")
ax = plt.subplot(111)
# Shrink current axis by 20%
box = ax.get_position()
ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
plt.legend(subj1, loc='center left', bbox_to_anchor=(1, 0.5), fontsize='small')
plt.savefig('result/plots/fig2/tpccbp_fig2.png')



