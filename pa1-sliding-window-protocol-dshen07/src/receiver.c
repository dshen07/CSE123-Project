#include "host.h"
#include <assert.h>
#include "switch.h"

void handle_incoming_frames(Host *host)
{
    // TODO: Suggested steps for handling incoming frames
    //    1) Dequeue the Frame from the host->incoming_frames_head
    //    2) Compute CRC of incoming frame to know whether it is corrupted
    //    3) If frame is corrupted, drop it and move on.
    //    4) Implement logic to check if the expected frame has come
    //    5) Implement logic to combine payload received from all frames belonging to a message
    //       and print the final message when all frames belonging to a message have been received.
    //    6) Implement the cumulative acknowledgement part of the sliding window protocol
    //    7) Append acknowledgement frames to the outgoing_frames_head queue
    int incoming_frames_length = ll_get_length(host->incoming_frames_head);
    while (incoming_frames_length > 0)
    {
        // Pop a node off the front of the link list and update the count
        LLnode *ll_inmsg_node = ll_pop_node(&host->incoming_frames_head);
        incoming_frames_length = ll_get_length(host->incoming_frames_head);
        Frame *inframe = ll_inmsg_node->value;
        // print_frame(inframe);
        uint8_t inframe_checksum = inframe->checksum;
        inframe->checksum = 0;
        uint8_t checksum = compute_crc8(convert_frame_to_char(inframe));
        if (checksum != inframe_checksum)
        {
            free(ll_inmsg_node);
            continue;
        }
        if (inframe->isACK == 1){
            ll_append_node(&host->incoming_acks_head, inframe);
            continue;
        }
        if (inframe->seq_num < host->LFR[inframe->src_id] || inframe->seq_num >= glb_sysconfig.window_size + host->LFR[inframe->src_id])
        {
            free(ll_inmsg_node);
            continue;
        }
        // printf("%d\n", inframe->seq_num);
        if (inframe->seq_num == host->LFR[inframe->src_id])
        {
            int totalACK = 0;
            host->receive_window_mask[inframe->src_id][0] = 1;
            host->receive_window[inframe->src_id][0].frame = inframe;
            for (int i = 0; i < glb_sysconfig.window_size; i++)
            {
                if (host->receive_window_mask[inframe->src_id][i] == 1)
                {
                    totalACK++;
                    Frame *currentFrame = host->receive_window[inframe->src_id][i].frame;
                    if (currentFrame->remaining_msg_bytes == 0)
                    {
                        if (host->currentString[inframe->src_id] == NULL)
                        {
                            printf("<RECV_%d>:[%s]\n", host->id, currentFrame->data);
                            host->currentString[inframe->src_id] = NULL;
                            host->stringLength[inframe->src_id] = 0;
                        }
                        else
                        {
                            char *temp = host->currentString[inframe->src_id];
                            int currentStringLength = host->stringLength[inframe->src_id];
                            char *message = malloc((currentStringLength + strlen(currentFrame->data)) * sizeof(char));
                            memcpy(message, temp, currentStringLength);
                            memcpy(message + currentStringLength, currentFrame->data, strlen(currentFrame->data));
                            printf("<RECV_%d>:[%s]\n", host->id, message);
                            host->currentString[inframe->src_id] = NULL;
                            host->stringLength[inframe->src_id] = 0;
                        }
                    }
                    else
                    {
                        if (host->currentString[inframe->src_id] == NULL)
                        {
                            char *temp = currentFrame->data;
                            host->currentString[inframe->src_id] = malloc(FRAME_PAYLOAD_SIZE * sizeof(char));
                            memcpy(host->currentString[inframe->src_id], temp, FRAME_PAYLOAD_SIZE);
                            host->stringLength[inframe->src_id] = FRAME_PAYLOAD_SIZE;
                        }
                        else
                        {
                            char *temp = host->currentString[inframe->src_id];
                            int currentStringLength = host->stringLength[inframe->src_id];
                            char *message = malloc((currentStringLength + FRAME_PAYLOAD_SIZE) * sizeof(char));
                            memcpy(message, temp, currentStringLength);
                            memcpy(message + currentStringLength, currentFrame->data, FRAME_PAYLOAD_SIZE);
                            host->currentString[inframe->src_id] = message;
                            host->stringLength[inframe->src_id] += FRAME_PAYLOAD_SIZE;
                        }
                    }
                }
                else
                {
                    break;
                }
            }
            for (int i = 0; i < glb_num_hosts; i++) {
                Host *h = &glb_hosts_array[i];
                h->LFR[inframe->src_id] = h->LFR[inframe->src_id] + totalACK;
            }
            // update receive window mask
            for (int i = 0; i < glb_sysconfig.window_size; i++)
            {
                if (i < totalACK)
                {
                    host->receive_window[inframe->src_id][i].frame = NULL;
                    host->receive_window_mask[inframe->src_id][i] = 0;
                }
                else
                {
                    if (host->receive_window_mask[inframe->src_id][i] == 1)
                    {
                        int newPosition = host->receive_window[inframe->src_id][i].frame->seq_num - host->LFR[inframe->src_id];
                        host->receive_window_mask[inframe->src_id][newPosition] = 1;
                        host->receive_window_mask[inframe->src_id][i] = 0;
                        host->receive_window[inframe->src_id][newPosition].frame = host->receive_window[inframe->src_id][i].frame;
                        host->receive_window[inframe->src_id][i].frame = NULL;
                    }
                }
            }
            // send ack
            Frame* ackFRAME = malloc(sizeof(Frame));
            ackFRAME->src_id = host->id;
            ackFRAME->dst_id = inframe->src_id;
            ackFRAME->remaining_msg_bytes = 0;
            ackFRAME->seq_num = 0;
            ackFRAME->isACK = 1;
            ackFRAME->ack = totalACK + inframe->seq_num - 1;
            strcpy(ackFRAME->data, "This is an ack frame.");
            ackFRAME->checksum = 0;
            ackFRAME->checksum = compute_crc8(convert_frame_to_char(ackFRAME));
            ll_append_node(&host->outgoing_frames_head, ackFRAME);
        }
        else
        {
            int position = inframe->seq_num - host->LFR[inframe->src_id];
            host->receive_window[inframe->src_id][position].frame = inframe;
            host->receive_window_mask[inframe->src_id][position] = 1;
            Frame* ackFRAME = malloc(sizeof(Frame));
            ackFRAME->src_id = host->id;
            ackFRAME->dst_id = inframe->src_id;
            ackFRAME->remaining_msg_bytes = 0;
            ackFRAME->seq_num = 0;
            ackFRAME->isACK = 1;
            ackFRAME->ack = host->LFR[inframe->src_id] - 1;
            strcpy(ackFRAME->data, "This is an ack frame.");
            ackFRAME->checksum = 0;
            ackFRAME->checksum = compute_crc8(convert_frame_to_char(ackFRAME));
            ll_append_node(&host->outgoing_frames_head, ackFRAME);
        }

        // printf("<RECV_%d>:[%s]\n", host->id, inframe->data);

        // free(inframe);
        free(ll_inmsg_node);
    }
}

void run_receivers()
{
    int recv_order[glb_num_hosts];
    get_rand_seq(glb_num_hosts, recv_order);

    for (int i = 0; i < glb_num_hosts; i++)
    {
        int recv_id = recv_order[i];
        handle_incoming_frames(&glb_hosts_array[recv_id]);
    }
}