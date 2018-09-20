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
#include "llvm/IR/Constants.h"



using namespace llvm;


namespace {
  struct Expression {

	std::string opcode;
	SmallVector<int, 4> numberingVector;
  Value* stored_value;
  
  	 };
  }
 
 
  


namespace {

int operandNo, loadnum, storenum = 0;
Expression *gexpr;


DenseMap<Value*, int> ValueNumberMap;  //record the value number against each variable 
DenseMap<Value*, Value*> ConstantMap;   
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

struct myclass {
  bool operator() (int i,int j) { return (i<j);}
} myobject;

	struct ExtendedLVN : public FunctionPass {
		static char ID;
		bool flag;
		bool bopFlag, exprFound, constant, no_optimization;
		Value *value_to_store;
		Value *lhs,*rhs;
		Value *new_binary;
		LoadInst *deleteload;
		
		
		ExtendedLVN() : FunctionPass(ID) {}
		bool runOnFunction(Function &F) {
			flag = false;
			bopFlag = false;
			exprFound = false;
			constant = false;
			
			for (BasicBlock &bb : F) {
				for (Instruction &i : bb) {
					
					if (BinaryOperator *bop = dyn_cast<BinaryOperator>(&i)) {
						bopFlag = true;
						Expression *expr = new Expression();				
						Value *operand;
						
						/*this for loop will get each operand and give them a value number if not already eexists. then it updates the expr->numberingVector*/
						for (unsigned OpNum = 0; OpNum < bop->getNumOperands(); ++OpNum) {
							operand = bop->getOperand(OpNum); 	//getting each operand of the binary operation
							expr->opcode = bop->getOpcodeName(); 	//getting the opcode opi
							
							/*To calculate the value number properly in case of a = 12 + c or a = c + 12*/
							if(isa<ConstantInt>(operand)){							
										errs() << "constant entered " << *operand << '\n';
										expr->numberingVector.push_back(-1); //check if this will actually work?? 
							} 
							//if the binary has load instruction for each operand in the binary instruction
							else if (LoadInst *op = dyn_cast<LoadInst>(operand)) {
								
								if(ConstantMap.lookup(op->getOperand(0))){
										errs() << "Found constant: " << *bop << "\n\n";
										loadnum = ValueNumberEntry(op->getOperand(0)); //getting and updating the value number of the load
										expr->numberingVector.push_back(loadnum);
										constant = true;
										deleteload = op;
										
										if(OpNum == 0){
											lhs = ConstantMap[op->getOperand(0)];
											rhs = bop->getOperand(OpNum + 1);
											
											
										}
										
										if(OpNum == 1){
											rhs = ConstantMap[op->getOperand(0)];
											lhs = bop->getOperand(OpNum - 1);
											
											
										}
											
								}else{								
								loadnum = ValueNumberEntry(op->getOperand(0)); //getting and updating the value number of the load
								
								expr->numberingVector.push_back(loadnum); //getting the Li and Ri and here I also have Opi	
								}						
							}
							
						}						
						/*Commutative operations -> only sorting when it's an add or mul*/
						if(expr->opcode == "add" or expr->opcode == "mul") 
							llvm::sort(expr->numberingVector.begin(), expr->numberingVector.end(), myobject);
						
								/*I know my hashkey li opi ri here*/
								
								//checking if the current expr is already in the map
						
						
						for (DenseMap< Expression*, int>::const_iterator it = ExpressionNumberMap.begin(), E = ExpressionNumberMap.end(); 								it != E; ++it){
						
							if(expr->numberingVector[0] == it->first->numberingVector[0] and expr->numberingVector[1] ==it->first->numberingVector[1] 									and expr->opcode == it->first->opcode ){ //match hashkey found --> need to replace it
							
								storenum = ExpressionNumberMap[it->first];//getting the Ti of the previously computed expr to later update it to 																																current Ti
								errs() << "storenum : " << storenum << "\n\n";
								value_to_store = it->first->stored_value; //this value needs to be stored in the address of duplicate expr
								
								errs() << "Same expr --> Value num : " << expr->numberingVector[0] << " " << expr->numberingVector[1] << 
								"\nCurrent Expr:" << *bop <<  "\nOriginal value is in:" << *value_to_store << "\n";
								
								exprFound = true;
								
								/*Here I have found a match and I also know what the original (it->first) expression stored its value to. So I need to erase the current instruction and and create load and store instructions so that it stores the value from destOperand. I probably need to do it in Store condition*/
							
							need_to_erase.push_back(bop);   //push the binary operation to deleted list
							
							/*push the load instructions of the operand to the deleted list*/
							for (unsigned OpNum = 0; OpNum < bop->getNumOperands(); ++OpNum) {
								if (LoadInst *op = dyn_cast<LoadInst>(bop->getOperand(OpNum)))
										need_to_erase.push_back(op);
							}
							break;
							
							}
						}
						
						/*creating the new binary operation if one of the operands found to be in ConstantMap*/
						
						if(constant && !exprFound){
							need_to_erase.push_back(deleteload);
							need_to_erase.push_back(bop);
							IRBuilder<> builder(bop);
							std::string code = bop->getOpcodeName();
							if(code == "add"){
									new_binary = builder.CreateNSWAdd(lhs, rhs);							
									errs() << "NewBinary" << *new_binary << "\n\n";
							}else if(code == "sub"){
									new_binary = builder.CreateNSWSub(lhs, rhs);							
									errs() << "NewBinary" << *new_binary << "\n\n";
							}
							
						}
						set(expr); //setting the pointer to hashkey
						
					}
					else if (StoreInst *op = dyn_cast<StoreInst>(&i)){
					
						
						Expression *expr = get(); //getting the expr object that was updated in load
						
						/*Check if the store instruction stores a constant*/
						if(isa<ConstantInt>(op->getOperand(0))){
							errs() << "Constant value : " << *op->getOperand(0) << " Constant stored to : " << *op->getOperand(1) << "\n\n";
							ConstantMap[op->getOperand(1)] = op->getOperand(0);
							errs() << "Value num" << operandNo << "\n\n";
						}
						
						if(constant && !exprFound){
							need_to_erase.push_back(op);
							IRBuilder<> builder(op);
							Instruction *store = builder.CreateAlignedStore(new_binary, op->getOperand(1), 4, false);
							errs() << "NewStore" << *store << "\n\n";
							constant = false;					
							
						}				
						
						/*checking if we have found the expression in ExpressionNumberMap. If it is a match we cannot increase the value number of Ti, rather have to use the old value number*/
						if(exprFound){
							Value *address_of_new_store = op->getOperand(1);	  //getting the address of current store so that we can store the previous 																																			value in it
							
							ValueNumberMap[address_of_new_store] = storenum; //updating the current Ti with the value of previous same expr's Ti
														
							need_to_erase.push_back(op); //push the store instruction for the redundant binary operation						
							
							errs() << "Need to store from originalSource to:" << *address_of_new_store << "\n\n";
							
							
							
							/*creating the new load and store instructions*/
							IRBuilder<> builder(op);
							
							Instruction *store = builder.CreateAlignedStore(value_to_store, address_of_new_store, 4, false);
							errs() << "NewStore" << *store << "\n\n";
						
							exprFound = false;
							
						}
						else{
							storenum = operandNo; //getting the Ti
							
							ValueNumberMap[op->getOperand(1)] = operandNo++; //updating the value number
							errs() << "Value num" << storenum << "\n\n";
						}
						
						/*to check that the store comes after a binary operation*/
						if(bopFlag){ 
							expr->stored_value = op->getOperand(0); //setting the value of the binary operation so that later we can replace with it
							ExpressionNumberMap[expr] = storenum; //constructing the map as expr as key and Ti as value
							bopFlag = false;								
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
char ExtendedLVN::ID = 0;

static void registerExtendedLVN(const PassManagerBuilder &, legacy::PassManagerBase &PM) {
	PM.add(new ExtendedLVN());
}

static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,registerExtendedLVN);
static RegisterPass<ExtendedLVN> X("extendedlvn","A Simple Algorithm for Local Value Numbering Pass",false,false);


