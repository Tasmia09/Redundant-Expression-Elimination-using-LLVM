#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/ADT/DenseMap.h"



using namespace llvm;


namespace {
  struct Expression {

	std::string opcode;
	SmallVector<int, 4> numberingVector;
	SmallVector<char, 4> opcodeVector;
  Value* stored_value;
  	 };
  }
 
 
  


namespace {

int operandNo, storenum = 0;
Expression *gexpr;


DenseMap<Value*, int> ValueNumberMap;  //record the value number against each variable 
DenseMap<Expression*, int> ExpressionNumberMap;  //record the hash_key of Li Opi Ri as key and value will be the value number where the 																										expression stores its value

SmallVector<Instruction*,8> need_to_erase;  //to record the instructions that we will need to erase


/*This function checks if variable has been recored with its value number. If yes, it returns the value number otherwise it enters its record to ValueNumberMap and returns the valuenumber*/

uint32_t ValueNumberEntry(Value *V) {
		//not found condition
		if(!ValueNumberMap.count(V)){
			
			ValueNumberMap[V] = operandNo++;
			return ValueNumberMap[V];
		}
		//found 
		return ValueNumberMap[V];
		
		
}

void set(Expression *ggexpr){
	gexpr = ggexpr;
	
}

Expression* get(){
	return gexpr;
}

	struct LVNV2 : public FunctionPass {
		static char ID;
		bool flag;
		bool bopFlag, exprFound, setExpr, extendedBopStart, currExtendedBop, extendedBopEnd;
		Value *value_to_store;
		
		Value *new_binary;
		Value *lhs,*rhs;
		
		LVNV2() : FunctionPass(ID) {}
		bool runOnFunction(Function &F) {
			flag = false;
			bopFlag = false;
			exprFound = false;
			setExpr = false;
			extendedBopStart = false;
			currExtendedBop = false;
			extendedBopEnd = false;
			
			for (BasicBlock &bb : F) {
				for (Instruction &i : bb) {
					
					if (BinaryOperator *bop = dyn_cast<BinaryOperator>(&i)) {
						bopFlag = true;
						Expression *expr = new Expression();				
						
						expr->opcode = bop->getOpcodeName();
						
						
						
						LoadInst *lop = dyn_cast<LoadInst>(bop->getOperand(0));
						LoadInst *rop = dyn_cast<LoadInst>(bop->getOperand(1));
						
						//Instruction next_instruction = i.getNextNode();
						if(lop and rop and !(isa<StoreInst>(i.getNextNode()))){
							extendedBopStart = true;
						}
						
						/*if both are load instructions*/
						if(lop and rop){
							/*getting the value numbers of Li and Ri --- if not exists create a new value number*/
							expr->numberingVector.push_back(ValueNumberEntry(lop->getOperand(0)));
							expr->numberingVector.push_back(ValueNumberEntry(rop->getOperand(0)));
							setExpr = true;			
						}else if(BinaryOperator *op = dyn_cast<BinaryOperator>(bop->getOperand(0))){
							errs() << "more than 2 operands "<< *op << "\n";
							
							expr->numberingVector.push_back(ValueNumberEntry(op)); //Li for the extended bop
							expr->numberingVector.push_back(ValueNumberEntry(rop->getOperand(0))); //Ri for the extended bop
							
							
							errs() << expr->numberingVector[0] << "----" << expr->numberingVector[1] << "\n\n";

							currExtendedBop = true;
							setExpr = true;
						}
						
						
						
						
						
						/*I know my hashkey li opi ri here*/
						
						if(setExpr){
							//checking if the current expr is already in the map
							for (DenseMap< Expression*, int>::const_iterator it = ExpressionNumberMap.begin(), E = ExpressionNumberMap.end(); 								it != E; ++it){
						
								if(expr->numberingVector[0] == it->first->numberingVector[0] and expr->numberingVector[1] ==it->first->numberingVector[1] 									and expr->opcode == it->first->opcode){ //match hashkey found --> need to replace it
							
								
									if(currExtendedBop){
										storenum = ExpressionNumberMap[it->first];//getting the Ti of the previously computed expr to later update it to 																																	current Ti
										value_to_store = it->first->stored_value; //this value needs to be stored in the address of duplicate expr
								
										errs() << "Same expr --> Value num : " << expr->numberingVector[0] << " " << expr->numberingVector[1] << 
										"\nCurrent Expr:" << *bop <<  "\nOriginal value is in:" << *value_to_store << "\n";
								
										exprFound = true;
										
										
										need_to_erase.push_back(bop); 
										BinaryOperator *dellop = dyn_cast<BinaryOperator>(bop->getOperand(0));
										LoadInst *delrop = dyn_cast<LoadInst>(bop->getOperand(1));
							
										
										
										need_to_erase.push_back(delrop);
										
										errs() << "============\n" << *bop << "\n"  << *bop->getOperand(1) << "\n" << "============\n";
								
									}
									
									else{
										storenum = ExpressionNumberMap[it->first];//getting the Ti of the previously computed expr to later update it to 																																	current Ti
										value_to_store = it->first->stored_value; //this value needs to be stored in the address of duplicate expr
								
										errs() << "Same expr --> Value num : " << expr->numberingVector[0] << " " << expr->numberingVector[1] << 
										"\nCurrent Expr:" << *bop <<  "\nOriginal value is in:" << *value_to_store << "\n";
								
										exprFound = true;
								
										/*Here I have found a match and I also know what the original (it->first) expression stored its value to. So I need to 									erase the current instruction and and create store instructions so that it stores the value from stored_value. I 									probably need to do it in Store condition*/
							
										need_to_erase.push_back(bop);   //push the binary operation to deleted list
							
										/*push the load instructions of the operand to the deleted list*/
										LoadInst *dellop = dyn_cast<LoadInst>(bop->getOperand(0));
										LoadInst *delrop = dyn_cast<LoadInst>(bop->getOperand(1));
							
										need_to_erase.push_back(dellop);
										need_to_erase.push_back(delrop);
										
										errs() << "============\n" << *bop << "\n" << *bop->getOperand(0) << "\n" << *bop->getOperand(1) << "\n" << "============\n";
							
									}
									break;
							
								}
							}
					}
					
					if(extendedBopStart or currExtendedBop){
								if(exprFound){
								errs() << expr->numberingVector[0] << "----" << expr->numberingVector[1] << "===" << storenum << "--->" << *bop << "\n\n";
								
								errs() << "Storing valunumber extended found" << storenum << " to " << *bop << "\n\n";
								ValueNumberMap[bop] = storenum;
								ExpressionNumberMap[expr] = storenum;
								expr->stored_value = value_to_store;
								extendedBopStart = false;
							
							}else{
								errs() << "extended but not found" << "\n";
								storenum = operandNo; //getting the Ti
								errs() << "Storing valunumber extended not found" << storenum << " to " << *bop << "\n\n";
								ValueNumberMap[bop] = operandNo++; //updating the value number
								ExpressionNumberMap[expr] = storenum;
								expr->stored_value = bop;
								errs() << expr->numberingVector[0] << "----" << expr->numberingVector[1] << "===" << storenum << "--->" << *bop << "\n\n";
								
								
							}
							extendedBopStart = false;
							}
							
						if(currExtendedBop and exprFound){
								
								lhs = value_to_store;
								rhs = bop->getOperand(1);
								IRBuilder<> builder(bop);
								//new_binary = builder.CreateNSWAdd(lhs, rhs);
								
							}
				
				set(expr); //setting the pointer to hashkey
				
			}
			else if (StoreInst *sop = dyn_cast<StoreInst>(&i)){
				Expression *expr = get(); //getting the expr object that was updated in load
				
				errs() << "\nStore instruction now is " << *sop << "\n";
				
				
				/*checking if we have found the expression in ExpressionNumberMap. If it is a match we cannot increase the value number of Ti, rather have to use the old value number*/
				if(exprFound){
				
				if(currExtendedBop){	
					
					errs() << "============\n" << *sop << "\n" << "============\n";
					
					need_to_erase.push_back(sop);
					IRBuilder<> builder(sop);
					/*Instruction *store = builder.CreateAlignedStore(new_binary, sop->getOperand(1), 4, false); //creating a store instruction to 																																																					   store the result to Ti
					
					expr->stored_value = store->getOperand(0);//setting the value of binary operation so that later we can replace with 																													it	*/
					Value *address_of_new_store = sop->getOperand(1);
					Instruction *store = builder.CreateAlignedStore(value_to_store, address_of_new_store, 4, false);			 	
					ExpressionNumberMap[expr] = storenum;    //constructing the hash key
					currExtendedBop = false;
					
					errs() <<  "New Store " << *store << "\n\n";
				}else{
					Value *address_of_new_store = sop->getOperand(1);	  //getting the address of current store so that we can store the previous 																																			value in it
					 						
					ValueNumberMap[address_of_new_store] = storenum; //updating the current Ti with the value of previous same expr's Ti
												
					errs() << "============\n" << *sop << "\n" << "============\n";
					need_to_erase.push_back(sop); //push the store instruction for the redundant binary operation						
					
					errs() << "Need to store from originalSource to:" << *address_of_new_store << "\n\n";
					
					
					
					/*creating the new load and store instructions*/
					IRBuilder<> builder(sop);
					/*Instruction *load = builder.CreateAlignedLoad(value_to_store, 4, "");							
					errs() << "NewLoad" << *load << "\n\n";*/
					
					builder.CreateAlignedStore(value_to_store, address_of_new_store, 4, false);
					
					exprFound = false;
					}
				}
				else if(!currExtendedBop){
					storenum = operandNo; //getting the Ti
					
					ValueNumberMap[sop->getOperand(1)] = operandNo++; //updating the value number
				}
				
				
				
				/*to check that the store comes after a binary operation*/
				if(bopFlag and setExpr){ 
					expr->stored_value = sop->getOperand(0);//setting the value of binary operation so that later we can replace with it
					 	
					ExpressionNumberMap[expr] = storenum; //constructing the map as expr as key and Ti as value
					
					errs() << "Storing valunumber " << storenum << " to " << *sop->getOperand(0) << "\n\n";
					bopFlag = false;
					setExpr = false;	
					currExtendedBop = false;							
				}
				
			}
		}
	}
	
				for (unsigned int i = 0; i < need_to_erase.size(); i++){
					need_to_erase[i]->eraseFromParent();
				}
			
			for (DenseMap< Expression*, int>::const_iterator it = ExpressionNumberMap.begin(), E = ExpressionNumberMap.end(); it != E; ++it){
				errs() << it->first->numberingVector[0] << " " << it->first->opcode << " " << it->first->numberingVector[1] << "  "<< ExpressionNumberMap[it->first] << "\n";		 //
			}
			
			/*for (DenseMap< Value*, int>::const_iterator it = ValueNumberMap.begin(), E = ValueNumberMap.end(); it != E; ++it){
				errs() << *it->first << "  "<< ValueNumberMap[it->first] << "\n";		 //
			}	*/
			
			
			
			flag = true;
			return flag;
		}
		
	};
}
char LVNV2::ID = 0;

static void registerLVNV2(const PassManagerBuilder &, legacy::PassManagerBase &PM) {
	PM.add(new LVNV2());
}

static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,registerLVNV2);
static RegisterPass<LVNV2> X("lvnv2","A Simple Algorithm for Local Value Numbering Pass",false,false);


