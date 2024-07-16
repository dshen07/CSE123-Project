#include "host.h"
#include "sender.h"
#include "receiver.h"
#include "switch.h"
#include <assert.h>

void init_host(Host *host, int id)
{
    host->id = id;
    host->active = 0;
    host->awaiting_ack = 0;
    host->round_trip_num = 0;
    host->csv_out = 0;
    host->next_seq = 0;
    host->LAR = 0;
    host->LFR = (int *)calloc(glb_num_hosts, sizeof(int));
    for (int i = 0; i < glb_num_hosts; i++)
    {
        host->LFR[i] = 0;
    }
    host->stringLength = (int *)calloc(glb_num_hosts, sizeof(int));
    host->currentString = (char **)malloc(glb_num_hosts * sizeof(char *));
    for (int i = 0; i < glb_num_hosts; i++)
    {
        host->currentString[i] = NULL;
    }
    host->receive_window = malloc(glb_num_hosts * sizeof(struct receive_window_slot *));
    for (int i = 0; i < glb_num_hosts; i++)
    {
        host->receive_window[i] = calloc(glb_sysconfig.window_size, sizeof(struct receive_window_slot));
        for (int j = 0; j < glb_sysconfig.window_size; j++)
        {
            host->receive_window[i][j].frame = NULL;
        }
    }
    host->receive_window_mask = (int **)malloc(glb_num_hosts * sizeof(int *));
    for (int i = 0; i < glb_num_hosts; i++)
    {
        host->receive_window_mask[i] = calloc(glb_sysconfig.window_size, sizeof(int));
    }

    host->incoming_acks_head = NULL;
    host->input_cmdlist_head = NULL;
    host->incoming_frames_head = NULL;
    host->buffered_outframes_head = NULL;
    host->outgoing_frames_head = NULL;
    host->send_window = calloc(glb_sysconfig.window_size, sizeof(struct send_window_slot));
    for (int i = 0; i < glb_sysconfig.window_size; i++)
    {
        host->send_window[i].frame = NULL;
        host->send_window[i].timeout = NULL;
    }
    host->latest_timeout = malloc(sizeof(struct timeval));
    gettimeofday(host->latest_timeout, NULL);

    // TODO: You should fill in this function as necessary to initialize variables

    // *********** PA1b ONLY ***********
    host->cc = calloc(glb_num_hosts, sizeof(CongestionControl));
    for (int i = 0; i < glb_num_hosts; i++)
    {
        host->cc[i].cwnd = 1.0;
        host->cc[i].ssthresh = (double)glb_sysconfig.window_size;
        host->cc[i].dup_acks = 0;
        host->cc[i].state = cc_SS;
    }
}

void run_hosts()
{
    run_senders();
    send_data_frames();
    run_receivers();
    send_ack_frames();
}