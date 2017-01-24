#include "procsim.hpp"

uint64_t R;
uint64_t F;
uint64_t J;
uint64_t K;
uint64_t L;

struct ScheQ* RS_head;
struct ScheQ* RS_tail;
int rsize;
int freeRS;


struct InsRecord* Rehead;  //each intru's stat
struct InsRecord* Retail;

int tag_count=0;

struct RegFile* RF;

struct CompQ* CPQ_head;
struct CompQ* CPQ_tail;
struct CompQ* CPQ_mid;
struct CompQ* CPQ_ahead_mid;

bool setmid;

int free[3];

struct ScheQ** retire;


#define RFSIZE 48

/**
 * Subroutine for initializing the processor. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @r Number of result buses
 * @k0 Number of k0 FUs
 * @k1 Number of k1 FUs
 * @k2 Number of k2 FUs
 * @f Number of instructions to fetch
 */
void setup_proc(uint64_t r, uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f) 
{
	R=r;
	J=k0;
	K=k1;
	L=k2;
	F=f;

	// DispQ_head = NULL;
	// DispQ_tail = NULL;

	rsize = 2*(J+K+L);
	freeRS = rsize;
	RS_head = NULL;
	RS_tail = NULL;

	// DispQ_head = NULL;
	// DispQ_tail = NULL;

	Rehead = NULL;
	Retail = NULL;

	RF = new RegFile[RFSIZE];
	for(int i=0;i<RFSIZE;i++){
		RF[i].ready = true;     //default init = 0;
		RF[i].tag = -1;
		//printf("RF%dready=%d  ",i,RF[i].ready);
	}


	CPQ_head = NULL;
	CPQ_tail = NULL;
	CPQ_mid = NULL;
	CPQ_ahead_mid = NULL;

	free[0] = J;
	free[1] = K;
	free[2] = L;

	retire = new ScheQ*[R];
	for(int i=0;i<R;i++){
		retire[i] = NULL;
	}

}



bool addto_RS(proc_inst_t inst,int32_t inst_num){

	if(freeRS==0){
		return false;
	}else{

		if(RS_head==NULL){
			RS_head = new ScheQ;
			RS_tail = RS_head;
			RS_tail->next = NULL;
		}else{
			
			ScheQ* tmp = new ScheQ;
			RS_tail->next = tmp;
			RS_tail = tmp;
			RS_tail->next = NULL;
		}

		RS_tail->func = inst.op_code;

		if(inst.src_reg[0]>0){
			RS_tail->src0_ready = RF[inst.src_reg[0]].ready;
			RS_tail->src0_tag = RF[inst.src_reg[0]].tag;
			
		}else{
			RS_tail->src0_ready = true;
			
		}

		if(inst.src_reg[1]>0){
			RS_tail->src1_ready = RF[inst.src_reg[1]].ready;
			RS_tail->src1_tag = RF[inst.src_reg[1]].tag;
			
		}else{
			RS_tail->src1_ready = true;
			
		}

		RS_tail->dest_reg = inst.dest_reg;

		if(RS_tail->dest_reg<0){
			RS_tail->dest_reg_tag = -1;
		}else{
			RS_tail->dest_reg_tag = tag_count;
			
			RF[inst.dest_reg].ready = false;
			RF[inst.dest_reg].tag = tag_count;

			tag_count++;
		}

		RS_tail->inst_num = inst_num;

		RS_tail->intoFU = false;

		freeRS--;
		return true;
	}

}


void addto_CPQ(struct ScheQ* theRS){


	if(CPQ_head==NULL){
		CPQ_head = new CompQ;
		CPQ_tail = CPQ_head;
		CPQ_tail->theRS = theRS;
		CPQ_tail->next = NULL;

		CPQ_mid = CPQ_tail;
	}else{
		CompQ* tmp = new CompQ;
		tmp->theRS = theRS;
		tmp->next = NULL;

		if(setmid){
			CPQ_ahead_mid = CPQ_tail;
			CPQ_mid = tmp;
			CPQ_tail->next = tmp;
			CPQ_tail = tmp;
		}else{
			CompQ* p1 = CPQ_ahead_mid;
			CompQ* p2 = CPQ_mid;
			bool flag=false;
			while(p2!=NULL){
				if((theRS->dest_reg_tag) < (p2->theRS->dest_reg_tag)){
					p1->next=tmp;
					tmp->next=p2;
					flag = true;
					break;
				}
				//both tag==-1?
				p1=p2;
				p2=p2->next;
			}
			if(flag==false){
				p1->next = tmp;
				tmp->next = NULL;
			}
		}


	}

}

struct ScheQ* removfrom_CPQ(){
	if(CPQ_head!=NULL){
		ScheQ* theRS = CPQ_head->theRS;
		CompQ* tmp = CPQ_head->next;
		delete CPQ_head;
		CPQ_head = tmp;
		return theRS;
	}else{
		return NULL;
	}
}


void addto_Record(int inst_num){

	if(Rehead==NULL){
		Retail = new InsRecord;
		Rehead = Retail;
		Retail->inst_num = inst_num;

		Retail->fetch=0;
		Retail->disp=0;
		Retail->sched=0;
		Retail->exec=0;
		Retail->state=0;
		Retail->finished=false;

		Retail->next = NULL;
	}else{
		Retail->next = new InsRecord;
		Retail = Retail->next;
		Retail->inst_num = inst_num;

		Retail->fetch=0;
		Retail->disp=0;
		Retail->sched=0;
		Retail->exec=0;
		Retail->state=0;
		Retail->finished=false;

		Retail->next = NULL;
	}

}

void removfrom_Record(int inst_num, int cycle){

	InsRecord* p=Rehead;

	while(p!= NULL){
		if(p->inst_num == inst_num){
			p->state=cycle+1;
			p->finished = true;
			break;
		}
		p=p->next;
	}

	if(p == Rehead){

		while(p!=NULL){
			if(p->finished){

				printf("%d\t%d\t%d\t%d\t%d\t%d\n",
					p->inst_num,p->fetch,p->disp,p->sched,p->exec,p->state);
				
				Rehead = Rehead->next;
				delete p;
				p = Rehead;

			}
			else{
				break;
			}

		}
	}

}



InsRecord* getrecord(int inst_num){
	InsRecord* tmp=Rehead;
	while(tmp!=NULL){
		if(tmp->inst_num==inst_num){
			return tmp;
		}
		tmp=tmp->next;
	}
}

/**
 * Subroutine that simulates the processor.
 *   The processor should fetch instructions as appropriate, until all instructions have executed
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */

void printqueue(){

	printf("scheQ: ");
	ScheQ* stmp=RS_head;
	while(stmp!=NULL){
		printf("%d-%d-%d   ",stmp->inst_num,stmp->src0_ready,stmp->src1_ready);
		stmp=stmp->next;
	}
	printf("\n");

	printf("compQ: ");
	CompQ* ctmp=CPQ_head;
	while(ctmp!=NULL){
		printf("%d ",ctmp->theRS->inst_num);
		ctmp=ctmp->next;
	}
	printf("\n");


}


int DispQ_length=0;


void run_proc(proc_stats_t* p_stats)
{	
	proc_inst_t inst;

	int PC = 1;

	InsRecord* tmprecord=NULL;

	int32_t ff;
	int32_t mask;
	ScheQ* rs1 = NULL;
	ScheQ* rs2 = RS_head;
	ScheQ* rebus[R];

	printf("INST\tFETCH\tDISP\tSCHED\tEXEC\tSTATE\n");

	p_stats->cycle_count=0;
	p_stats->retired_instruction=0;
	p_stats->max_disp_size=0;
	p_stats->avg_disp_size=0;
	p_stats->avg_inst_fired=0;
	p_stats->avg_inst_retired=0;
	unsigned long dispsize_sum=0;
	unsigned int read_instr_count=0;
	int last_fetch_cycle=0;
	DispQ* Dhead = new DispQ;
	Dhead->size=0;
	Dhead->cycle=0;
	Dhead->next=NULL;
	DispQ* Dtail = Dhead;

	for(int cycle=1;cycle>0;cycle++){

		//printf("cycle %d\n",cycle);
		p_stats->cycle_count++;
		
		//------------------exec (put on bus)------------------
		

		for(int i=0;i<R;i++){
			rebus[i] = NULL;
			rebus[i] = removfrom_CPQ();
			if(rebus[i]==NULL){
				break;
			}

			ff = rebus[i]->func;
			mask = ff >> 31;
			ff = ff ^ mask;
			ff = ff - mask;
			free[ff]++;

			int32_t dest_reg = rebus[i]->dest_reg;
			int32_t dest_reg_tag = rebus[i]->dest_reg_tag;

			int32_t inst_num = rebus[i]->inst_num;

			if(RF[dest_reg].tag == dest_reg_tag){
				RF[dest_reg].ready = true;
			}

			removfrom_Record(inst_num,cycle);
			p_stats->retired_instruction ++;
			//printf("haha\n");

		}

		

		//-------------------------------------------

		//-------------------enter FU (addto_CompQ) (+1 = exec)--------------------

		rs1 = NULL;
		rs2 = RS_head;

		int flag=0;
		while(rs2!=NULL){
			if((rs2->src0_ready) && (rs2->src1_ready) && (!rs2->intoFU)){

				ff = rs2->func;
				mask = ff >> 31;
				ff = ff ^ mask;
				ff = ff - mask;

				if(free[ff]>0){
					rs2->intoFU = true;

					if(flag==0){
						setmid = true;
					}
					addto_CPQ(rs2);

					flag++;

					free[ff]--;
					tmprecord = getrecord(rs2->inst_num);
					tmprecord->exec = cycle+1;
				}
			}
			rs2 = rs2->next;
		}

	
		//printf("cycle %d\n",cycle);
		//-------------------------------------------------------------------------


		//---------------------update RS (mark ready)------------------

		for(int i=0;i<R;i++){
			if(rebus[i]==NULL){
				break;
			}
			ScheQ* tmp = RS_head;
			rs2 = NULL;
			rs1 = NULL;
			while(tmp!=NULL){
				if(tmp==rebus[i]){
					rs2 = tmp;
					//printf("hhaha\n");
				}
				else {
					if(tmp->src0_ready == false){
						if(tmp->src0_tag == rebus[i]->dest_reg_tag){
							tmp->src0_ready = true;
						}
					}
					if(tmp->src1_ready == false){
						if(tmp->src1_tag == rebus[i]->dest_reg_tag){
							tmp->src1_ready = true;
						}
					}
				}

				//increment
				if(rs2==NULL){
					rs1 = tmp;
				}
				
				tmp = tmp->next;
					
			}

		}
		
		//-------------------------------------------------------------------------


		//--------------enter RS (+1 =sched)-----------------------

		while(freeRS>0){
			if((DispQ_length>0)){

				if(read_instruction(&inst)){

					read_instr_count++;

					addto_RS(inst,PC); 
					addto_Record(PC);
					//printf("ha cycle%d\n",cycle);
					
					Retail->sched = cycle+1;
					Retail->fetch = (PC-1)/4 + 1;
					Retail->disp = Retail->fetch + 1;
				
					DispQ_length--;

					PC++;
				}else{
					//dispq empty
					DispQ_length = -1;
					last_fetch_cycle = read_instr_count/4;
					DispQ* tmp = Dhead;

					for(int i=0;i>-1;i++){

						if (tmp->cycle == cycle-1){
							dispsize_sum += tmp->size - 4*(tmp->cycle - last_fetch_cycle);
							break;
						}
						if (tmp->cycle <= last_fetch_cycle){
							
							dispsize_sum += tmp->size;
							if(p_stats->max_disp_size < tmp->size){
								p_stats->max_disp_size = tmp->size;
							}
						}else{
							dispsize_sum += tmp->size - 4*(tmp->cycle - last_fetch_cycle);
						}
						
						tmp=tmp->next;
					}
					break;
				}

			}else{
				break;
			}
		}
		
		if(DispQ_length > 0){

			Dtail->next = new DispQ;
			Dtail = Dtail->next;
			Dtail->size = cycle*4 - read_instr_count;
			Dtail->cycle = cycle;
			Dtail->next =NULL;

		}
		

		

		//---------------------------------------------------------

		//---------------------delete from RS----------------------

		for(int i=0;i<R;i++){
			if(retire[i]==NULL){
				break;
			}
			ScheQ* tmp = RS_head;
			rs2 = NULL;
			rs1 = NULL;
			while(tmp!=NULL){
				if(tmp==retire[i]){
					rs2 = tmp;
					//printf("hhaha\n");
				}
				//increment
				if(rs2==NULL){
					rs1 = tmp;
				}
				
				tmp = tmp->next;
					
			}

			if((rs1==NULL) && (rs2!=NULL)){
				RS_head = rs2->next;
				delete rs2;
				freeRS++;
				//printf("ho cycle%d\n",cycle);
			}
			if((rs1!=NULL) && (rs2!=NULL)){
				rs1->next = rs2->next;
				delete rs2;
				freeRS++;
				//printf("ho cycle%d\n",cycle);
			}
			
		}
		for(int i=0;i<R;i++){
			retire[i]=rebus[i];
		}


		//printf("cycle %d\n",cycle);
		//---------------------------------------------------------

		//--------------fetch (enter dispQueue)-----------------------
		//--------------disp (fetch + 1 = disp)-----------------------
		if(DispQ_length>=0){
			DispQ_length += F;
		}
		

		// if(p_stats->max_disp_size < DispQ_length){
		// 	p_stats->max_disp_size = DispQ_length;
		// }
		
		if((DispQ_length==-1)&&(RS_head==NULL)&&(CPQ_head==NULL)){
			cycle=-10;
		}


	}

	p_stats->avg_disp_size = (dispsize_sum+0.0)/(p_stats->cycle_count);
	p_stats->avg_inst_fired = ((float)p_stats->retired_instruction+0.0)/p_stats->cycle_count ;
	p_stats->avg_inst_retired = p_stats->avg_inst_fired;
	
}



/**
 * Subroutine for cleaning up any outstanding instructions and calculating overall statistics
 * such as average IPC, average fire rate etc.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_proc(proc_stats_t *p_stats) 
{
	printf("\n");

}
