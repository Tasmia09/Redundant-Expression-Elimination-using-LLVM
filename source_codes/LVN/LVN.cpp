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

	struct LVN : public FunctionPass {
		static char ID;
		bool flag;
		bool bopFlag, exprFound, setExpr;
		Value *value_to_store;
		
		
		LVN() : FunctionPass(ID) {}
		bool runOnFunction(Function &F) {
			flag = false;
			bopFlag = false;
			exprFound = false;
			setExpr = false;
			
			for (BasicBlock &bb : F) {
				for (Instruction &i : bb) {
					
					if (BinaryOperator *bop = dyn_cast<BinaryOperator>(&i)) {
						bopFlag = true;
						Expression *expr = new Expression();				
						
						expr->opcode = bop->getOpcodeName();
						
						
						
						LoadInst *lop = dyn_cast<LoadInst>(bop->getOperand(0));
						LoadInst *rop = dyn_cast<LoadInst>(bop->getOperand(1));
						
						/*if both are load instructions*/
						if(lop and rop){
							/*getting the value numbers of Li and Ri --- if not exists create a new value number*/
							expr->numberingVector.push_back(ValueNumberEntry(lop->getOperand(0)));
							expr->numberingVector.push_back(ValueNumberEntry(rop->getOperand(0)));
							setExpr = true;			
						}
						
						/*I know my hashkey li opi ri here*/
						
						if(setExpr){
							//checking if the current expr is already in the map
							for (DenseMap< Expression*, int>::const_iterator it = ExpressionNumberMap.begin(), E = ExpressionNumberMap.end(); 								it != E; ++it){
						
								if(expr->numberingVector[0] == it->first->numberingVector[0] and expr->numberingVector[1] ==it->first->numberingVector[1] 									and expr->opcode == it->first->opcode){ //match hashkey found --> need to replace it
							
								
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
							
									break;
							
								}
							}
					}
				
				set(expr); //setting the pointer to hashkey
				
			}
			else if (StoreInst *sop = dyn_cast<StoreInst>(&i)){
				Expression *expr = get(); //getting the expr object that was updated in load
				
				errs() << "\nStore instruction now is " << *sop << "\nValue of exprfound " << exprFound << "\nValue of bopglag " << bopFlag <<"\nValue of setExpr " << setExpr << "\n\n";
				
				
				/*checking if we have found the expression in ExpressionNumberMap. If it is a match we cannot increase the value number of Ti, rather have to use the old value number*/
				if(exprFound){
					Value *address_of_new_store = sop->getOperand(1);	  //getting the address of current store so that we can store the previous 																																			value in it
					 						
					ValueNumberMap[address_of_new_store] = storenum; //updating the current Ti with the value of previous same expr's Ti
												
					need_to_erase.push_back(sop); //push the store instruction for the redundant binary operation						
					
					errs() << "Need to store from originalSource to:" << *address_of_new_store << "\n\n";
					
					
					
					/*creating the new load and store instructions*/
					IRBuilder<> builder(sop);
					/*Instruction *load = builder.CreateAlignedLoad(value_to_store, 4, "");							
					errs() << "NewLoad" << *load << "\n\n";*/
					
					builder.CreateAlignedStore(value_to_store, address_of_new_store, 4, false);
					
					exprFound = false;
					
				}
				else{
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
			
			for (DenseMap< Value*, int>::const_iterator it = ValueNumberMap.begin(), E = ValueNumberMap.end(); it != E; ++it){
				errs() << *it->first << "  "<< ValueNumberMap[it->first] << "\n";		 //
			}	
			
			
			
			flag = true;
			return flag;
		}
		
	};
}
char LVN::ID = 0;

static void registerLVN(const PassManagerBuilder &, legacy::PassManagerBase &PM) {
	PM.add(new LVN());
}

static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,registerLVN);
static RegisterPass<LVN> X("lvn","A Simple Algorithm for Local Value Numbering Pass",false,false);


