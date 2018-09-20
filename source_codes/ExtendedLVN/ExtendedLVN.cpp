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
	SmallVector<int, 4> numberingVector; //to store the value numbers of this expressions's operands
  Value* stored_value;
  	 };
  }
 
 
  


namespace {

int operandNo, storenum = 0;
Expression *gexpr;


DenseMap<Value*, int> ValueNumberMap;  //record the value number against each variable 
DenseMap<Value*, Value*> ConstantMap;  //record the constant value address as key and actual value as value
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

int getNumericValue(Value *v){	
	if (ConstantInt* CI = dyn_cast<ConstantInt>(v)) {
		if (CI->getBitWidth() <= 32) {
			return CI->getSExtValue();
		}
	}
} 

struct myclass {
  bool operator() (int i,int j) { return (i<j);}
} myobject;


	struct ExtendedLVN : public FunctionPass {
		static char ID;
		bool flag;
		bool bopFlag, exprFound, constant, setExpr, setIdentity, storeZero, storeOne, eraseConstant;
		Value *value_to_store;
		Value *new_binary;
		Value *lhs,*rhs;
		
		
		ExtendedLVN() : FunctionPass(ID) {}
		bool runOnFunction(Function &F) {
			flag = false;
			bopFlag = false;  
			exprFound = false;
			constant = false;
			setExpr = false;
			setIdentity = false;
			storeZero = false;
			storeOne = false;
			eraseConstant = false;
			
			for (BasicBlock &bb : F) {
				errs() << F.getName() << "\n\n";
				operandNo = 0;
				for (Instruction &i : bb) {
					
					if (BinaryOperator *bop = dyn_cast<BinaryOperator>(&i)) {
								
						bopFlag = true; 
						Expression *expr = new Expression();				
						
						
						expr->opcode = bop->getOpcodeName();
						
						LoadInst *lop = dyn_cast<LoadInst>(bop->getOperand(0));
						LoadInst *rop = dyn_cast<LoadInst>(bop->getOperand(1));
						
						/*checking if both operand is load -> this check is needed for determining identity operation*/
						if (lop and rop){
						
								/* give them a value number if not already exists and it updates the expr->numberingVector*/
								//made changes here that have not been tested
								expr->numberingVector.push_back(ValueNumberEntry(lop->getOperand(0)));
								expr->numberingVector.push_back(ValueNumberEntry(rop->getOperand(0)));
														
								/*if both the operand is same and the operator is either sub or div --> a-a=0 or a/a=1*/
								if(ValueNumberMap.lookup(lop->getOperand(0)) == ValueNumberMap.lookup(rop->getOperand(0)) and (expr->opcode == "sub" or expr->opcode == "sdiv")){
									if(expr->opcode == "sub"){											
										need_to_erase.push_back(bop);
										need_to_erase.push_back(lop);
										need_to_erase.push_back(rop);											
										storeZero = true;
									}else if(expr->opcode == "sdiv"){
										need_to_erase.push_back(bop);
										need_to_erase.push_back(lop);
										need_to_erase.push_back(rop);
										storeOne = true;
									}											
								}else{
									/*this means both operand is load and we dont have any identity match--so getting the value number for Li and Ri when 											both the operand is a load instruction and they are not same operand*/
									/*
									expr->numberingVector.push_back(ValueNumberEntry(lop->getOperand(0)));
									expr->numberingVector.push_back(ValueNumberEntry(rop->getOperand(0)));
									*/
									setExpr = true;
									
									//checking if Li and Ri are both constants
									if(ConstantMap.lookup(lop->getOperand(0)) && ConstantMap.lookup(rop->getOperand(0))){
										errs() << "both constant " << *lop->getOperand(0) << "   " << *rop->getOperand(0) << "\n";
										constant = true;
									}else{
										constant = false;
										eraseConstant = true;
									}
								}
						}else if(lop){
								/*left operand is load -- we will not construct a hash key for this expression and we will check if Ri falls under 											certain values to match the identity*/
								setExpr = false;
								int constIntValue;
								Value *v = bop->getOperand(1);
								constIntValue = getNumericValue(v);
								
								/*checking for a*0=0 */
								if(constIntValue == 0 and expr->opcode == "mul"){
									need_to_erase.push_back(bop);	
									need_to_erase.push_back(lop);
									storeZero = true;
								}	
								/*checking for a+0=a, a-0=a, a*1=a, a/1=a */									
								else if((constIntValue == 0 and (expr->opcode == "add" or expr->opcode == "sub")) or (constIntValue == 1 and 											(expr->opcode == "mul" or expr->opcode == "sdiv"))){
									value_to_store = lop;
									need_to_erase.push_back(bop);	
									setIdentity = true;
								}
						}else if(rop){
							/*right operand is load -- we will not construct a hash key for this expression and we will check if Li falls under 									certain values to match the identity*/
							setExpr = false;
							int constIntValue;
							Value *v = bop->getOperand(0);
							constIntValue = getNumericValue(v);
							
							/*checking for 0*a=0 */
							if(constIntValue == 0 and expr->opcode == "mul"){
								need_to_erase.push_back(bop);	
								need_to_erase.push_back(rop);
								storeZero = true;
							}
							/*checking for 0+a=a, 1*a=a */
							else if((constIntValue == 0 and expr->opcode == "add") or (constIntValue == 1 and expr->opcode == "mul")){ 
								value_to_store = rop;
								need_to_erase.push_back(bop);	
								setIdentity = true;
							}
						}
						
						/*Commutative operations -> only sorting when it's an add or mul and both operand is a load and does not match identity*/
						if((expr->opcode == "add" or expr->opcode == "mul") and setExpr){ 
							llvm::sort(expr->numberingVector.begin(), expr->numberingVector.end(), myobject);
						}
				
						/*I know my hashkey li opi ri here*/
						
						//checking if the current expr is supposed to construct a hash key and if it is already in the map
						if(setExpr){
						for (DenseMap< Expression*, int>::const_iterator it = ExpressionNumberMap.begin(), E = ExpressionNumberMap.end(); 								it != E; ++it){
						
							if(expr->numberingVector[0] == it->first->numberingVector[0] and expr->numberingVector[1] ==it->first->numberingVector[1] 									and expr->opcode == it->first->opcode){ //match hashkey found --> need to replace it
							
								storenum = ExpressionNumberMap[it->first];//getting the Ti of the previously computed expr to later update it to 																																	current Ti
								value_to_store = it->first->stored_value; //this value needs to be stored in the address of duplicate expr
								
								
								/*check that if the duplicate expression is actaully a constant expression*/
								if(isa<ConstantInt>(value_to_store)){
										errs() << "Duplicate and constant found " << "\n";
										constant = true;
								}
								
								errs() << "Same expr --> Value num : " << expr->numberingVector[0] << " " << expr->opcode << " "<< expr->numberingVector[1] << "\nCurrent Expr: -- " << *bop <<  "\nOriginal value is in:  " << *value_to_store << "\n";
								
								exprFound = true;
								
								/*Here I have found a match and I also know what the original (it->first) expression stored its value to. So I need to erase the current binary instruction and and create load and store instructions so that it stores the value from destOperand. I probably need to do it in Store condition*/
							
							need_to_erase.push_back(bop);   //push the binary operation to deleted list
							//need_to_erase.push_back(lop);		//push the load instructions of the operand to the deleted list
							//need_to_erase.push_back(rop);
							
							/*push the load instructions of the operand to the deleted list*/
							
							LoadInst *dellop = dyn_cast<LoadInst>(bop->getOperand(0));
							LoadInst *delrop = dyn_cast<LoadInst>(bop->getOperand(1));
							
							need_to_erase.push_back(dellop);
							need_to_erase.push_back(delrop);
							/*for (unsigned OpNum = 0; OpNum < bop->getNumOperands(); ++OpNum) {
								if (LoadInst *op = dyn_cast<LoadInst>(bop->getOperand(OpNum)))
										need_to_erase.push_back(op);
							}*/
							break;
							
							}
						}
						
					}
				
				/*if the current expression has constant operands and is not a duplicate instruction*/		
				if(constant and !exprFound){
						need_to_erase.push_back(bop);
						/*getting the lhs and rhs of the constant expression so that we can create the new binary*/
						//lhs = ConstantMap[lop->getOperand(0)];
						//rhs = ConstantMap[rop->getOperand(0)];
						//need_to_erase.push_back(lop);
						//need_to_erase.push_back(rop);
						for (unsigned OpNum = 0; OpNum < bop->getNumOperands(); ++OpNum) {
							if (LoadInst *op = dyn_cast<LoadInst>(bop->getOperand(OpNum))){
									need_to_erase.push_back(op);
									if(OpNum == 0)
										lhs = ConstantMap[op->getOperand(0)];
									else if(OpNum == 1)
										rhs = ConstantMap[op->getOperand(0)];
							}
						}
							
					/*evaluate Li Opi Ri store the result so that we can store it to Ti*/
					IRBuilder<> builder(bop);
					std::string code = bop->getOpcodeName();
					if(code == "add"){
							new_binary = builder.CreateNSWAdd(lhs, rhs);
					}else if(code == "sub"){
							new_binary = builder.CreateNSWSub(lhs, rhs);							
					}else if(code == "mul"){
							new_binary = builder.CreateMul(lhs, rhs);							
					}else if(code == "sdiv"){
							new_binary = builder.CreateSDiv(lhs, rhs);							
					}		
				}
				
				set(expr); //setting the pointer to hashkey
				
			}
			/*for store instructions*/
			else if (StoreInst *sop = dyn_cast<StoreInst>(&i)){	
				Expression *expr = get(); //getting the expr object that was updated in load
				
				if(storeZero){
					storeZero = false;
					need_to_erase.push_back(sop);
					IRBuilder<> builder(sop);
					auto* zero = builder.getInt32(0);
          builder.CreateAlignedStore(zero, sop->getOperand(1), 4, false); //creating a store instruction to set the value as 0
          ConstantMap[sop->getOperand(1)] = zero; //since its value is now zero it is a constant
				}else if(storeOne){
					storeOne = false;
					need_to_erase.push_back(sop);
					IRBuilder<> builder(sop);
					auto* one = builder.getInt32(1);;
					builder.CreateAlignedStore(one, sop->getOperand(1), 4, false); //creating a store instruction to set the value as 1
					ConstantMap[sop->getOperand(1)] = one; //since its value is now one it is a constant
				}else if(setIdentity){
					setIdentity = false;
					need_to_erase.push_back(sop);
					IRBuilder<> builder(sop);
					builder.CreateAlignedStore(value_to_store, sop->getOperand(1), 4, false); //creating a store instruction to
					if(ConstantMap.lookup(sop->getOperand(0)))
						ConstantMap[sop->getOperand(1)] = ConstantMap[sop->getOperand(0)]; 
				}
				
				/*check if this store is after a both constant opernad expression and not a duplicate expression*/
				else if(constant && !exprFound){
					need_to_erase.push_back(sop);
					IRBuilder<> builder(sop);
					Instruction *store = builder.CreateAlignedStore(new_binary, sop->getOperand(1), 4, false); //creating a store instruction to 																																																					   store the result to Ti
					ConstantMap[store->getOperand(1)] = store->getOperand(0); //if both are constant Ti is constant, so marking Ti as constant
					
					errs() << "Now constant " << *store->getOperand(1) << "\n\n";
					expr->stored_value = store->getOperand(0);//setting the value of binary operation so that later we can replace with 																													it				 	
					ExpressionNumberMap[expr] = operandNo;    //constructing the hash key ??? how is this not operandNo ++ ???
					constant = false;					
					bopFlag = false;
				}
				/*Check if the store instruction stores a value that loads a constant*/ /*checckkkk*/
				else if (LoadInst *lop = dyn_cast<LoadInst>(sop->getOperand(0))) {
				 errs() << "Inside store and load checking  ===============" << *sop->getOperand(1) <<  "\n";
					if(ConstantMap.lookup(lop->getOperand(0))){
						errs() << "Checkin if the store instruction stores a value that loads a constant---\n" << *lop->getOperand(0) << "\n\n";
						ConstantMap[sop->getOperand(1)] = ConstantMap[lop->getOperand(0)]; //key is the constant address and value is the constant value
						}
					else if(ConstantMap.lookup(sop->getOperand(1))){
						errs() << "Erasing   " << *sop->getOperand(1) << "\n\n";
						ConstantMap.erase(sop->getOperand(1));
					}
				}
				/*if the store instruction stores a constant*/
				else if(isa<ConstantInt>(sop->getOperand(0))){
					errs() << "store instruction stores a constant " <<"\n"; 
					ConstantMap[sop->getOperand(1)] = sop->getOperand(0); //key is the constant address and value is the constant value
				}
				else if(eraseConstant){
					if(ConstantMap.lookup(sop->getOperand(1))){
						errs() << "Erasing   " << *sop->getOperand(1) << "\n\n";
						ConstantMap.erase(sop->getOperand(1));
					}
					eraseConstant = false;
				}
				
				
				
				/*checking if we have found the expression in ExpressionNumberMap. If it is a match we cannot increase the value number of Ti, rather have to use the old value number*/
				if(exprFound){
				
					/*if the duplicate instruction is a constant expression*/
					/*check if works*/
					if(constant){
						errs() << "Duplicate and constant" << "\n";
						ConstantMap[sop->getOperand(1)] =  value_to_store;  //marking Ti as constant
						errs() << "Now constant " << *sop->getOperand(1) << "\n\n";
						constant = false;
					}
						
					Value *address_of_new_store = sop->getOperand(1);	  //getting the address of current store so that we can store the previous 																																			value in it
					 						
					ValueNumberMap[address_of_new_store] = storenum; //updating the current Ti with the value of previous same expr's Ti
												
					need_to_erase.push_back(sop); //push the store instruction for the redundant binary operation						
					
					/*creating the new load and store instructions*/
					IRBuilder<> builder(sop);
					//Instruction *load = builder.CreateAlignedLoad(value_to_store, 4, "");							
					
					
					//builder.CreateAlignedStore(load, address_of_new_store, 4, false);
					
				
					builder.CreateAlignedStore(value_to_store, address_of_new_store, 4, false);
					exprFound = false;
					
				}else{
					storenum = operandNo; //getting the Ti
					
					ValueNumberMap[sop->getOperand(1)] = operandNo++; //updating the value number
					
					errs() << storenum << " is for " << *sop->getOperand(1) << "------------" << *sop << "\n\n\n"; 
				}
				
				/*to check that the store comes after a binary operation*/
				if(bopFlag and setExpr){ 
					expr->stored_value = sop->getOperand(0);//setting the value of binary operation so that later we can replace with it
										 	
					ExpressionNumberMap[expr] = storenum; //constructing the map as expr as key and Ti as value
					bopFlag = false;
					setExpr = false;								
				}
				
				
				
				
			}
			
		
		}
		
		
				
			for (DenseMap< Expression*, int>::const_iterator it = ExpressionNumberMap.begin(), E = ExpressionNumberMap.end(); it != E; it++){
				errs() << it->first->numberingVector[0] << " " << it->first->opcode << " " << it->first->numberingVector[1] << "  "<< 			ExpressionNumberMap[it->first] << " : " << *it->first->stored_value << "\n";		 //
			}
			
			for (DenseMap< Value*, int>::const_iterator it = ValueNumberMap.begin(), E = ValueNumberMap.end(); it != E; ++it){
				errs() << *it->first << "  "<< ValueNumberMap[it->first] << "\n";		 //
			}
			
			errs() << "Constant Map" << "\n\n"; 
			for (DenseMap< Value*, Value*>::const_iterator it = ConstantMap.begin(), E = ConstantMap.end(); it != E; ++it){
				errs() << *it->first << " : "<< 			*ConstantMap[it->first] << "\n";		 //
			}	
				
			for (unsigned int i = 0; i < need_to_erase.size(); i++){
				need_to_erase[i]->eraseFromParent();
			}
			
				ValueNumberMap.clear();
				ConstantMap.clear();
				ExpressionNumberMap.clear();
				need_to_erase.clear();
				
				
				
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


