import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

gptm = np.genfromtxt('result/data/tpcc_f2.txt', delimiter=',', usecols=np.arange(0,7))
gcgl = np.genfromtxt('result/data/tpcc_f2.txt', delimiter=',', usecols=7)
iptm = np.genfromtxt('result/data/tpcc_f2.txt', delimiter=',', usecols=np.arange(8,15))
icgl = np.genfromtxt('result/data/tpcc_f2.txt', delimiter=',', usecols=15)
x = np.array([1,7,14,20,27,34,41,48])
plt.xlim(0,50)
plt.xticks(x,['1', '2', '4', '8', '16', '24', '32', '48'])

subj1=["pg_cgl_eager", "pg_orec_eager", "pg_orec_lazy", "pg_orec_mixed", "pg_norec", "pg_ring_sw", "pg_tlrw_eager", "pg_cgl_lazy", "pi_cgl_eager", "pi_orec_eager", "pi_orec_lazy", "pi_orec_mixed", "pi_norec", "pi_ring_sw", "pi_tlrw_eager", "pi_cgl_lazy"]

plt.plot(x,gptm)
plt.plot(x,gcgl, color='dodgerblue')
plt.plot(x,iptm, linestyle='dashed')
plt.plot(x,icgl, linestyle='dashed', color='dodgerblue')
plt.title("TPCC")
plt.xlabel("Number of Threads")
plt.ylabel("Throughput (Ktps)")
ax = plt.subplot(111)
# Shrink current axis by 20%
box = ax.get_position()
ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
plt.legend(subj1, loc='center left', bbox_to_anchor=(1, 0.5), fontsize='small')
plt.savefig('result/plots/fig3/tpcc_fig3.png')


