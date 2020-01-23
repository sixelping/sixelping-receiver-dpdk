//
// Created by rewbycraft on 1/20/20.
//

#include <rte_mempool.h>
#include <rte_mbuf.h>

void nipktmfree(struct rte_mbuf *m) {
	rte_pktmbuf_free(m);
}

