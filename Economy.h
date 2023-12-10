#pragma once

class MargDisUtilLabour {
public:
	float a = 50.0f; // scale disutility
	float disUtility(float quantity_labour) {
		return ((-1.0f * a) / (quantity_labour - 16.0f)) - ((-1.0f * a) / 16.0f);
	}
};

class MargUtil {
public:
	float a = 1.0f; // scale utility
	float b = 0.5f; // rate drop off
	float utility(float quantity_good) {
		return a * pow(2.0f, -1.0f * b * quantity_good);
	}
};

class ProdFunc {
public:
	float a = 1.0f; // scale production
	float supply(float quantity_labour) {
		return a * sqrt(quantity_labour);
	}
	float dSdL(float quantity_labour) {
		return a / (2 * sqrt(quantity_labour));
	}
};

class Agent {
public:
	int index = -1;

	int numAgents = -1;
	int numGoods = -1;

	int most_labour_productive_good = -1;

	float labour_commitment = 8.0f;
	float income = 0.0f;
	float wage_rate = 0.0f;
	MargDisUtilLabour marg_dis_util_labour;
	vector<MargUtil> marg_utilities;
	vector<ProdFunc> prod_functions;

	vector<float> prices;
	vector<float> supplies;
	vector<float> labour_allocations;
	vector<float> market_revenues;
	vector<float> dVdL;
	vector<float> consumptions;

	vector<vector<float>> budget_allocations;
	vector<vector<float>> agent_spendings;

	Agent(int numAgents, int numGoods, int index) {
		this->index = index;
		this->numAgents = numAgents;
		this->numGoods = numGoods;

		for (int i = 0; i < numGoods; i++) {
			marg_utilities.push_back(MargUtil());
			prod_functions.push_back(ProdFunc());

			prices.push_back(1.0f);
			supplies.push_back(1.0f);
			labour_allocations.push_back(1.0f / numGoods);
			market_revenues.push_back(100.0f);
		}

		for (int i = 0; i < numAgents; i++) {
			vector<float> goods;
			for (int i = 0; i < numGoods; i++) {
				goods.push_back(1.0f / (numAgents * numGoods));
			}
			budget_allocations.push_back(goods);
		}
		for (int i = 0; i < numAgents; i++) {
			vector<float> goods;
			for (int i = 0; i < numGoods; i++) {
				goods.push_back(0.0f);
			}
			agent_spendings.push_back(goods);
		}
	}
};

class EconomicSimulation {
public:
	int numAgents = -1;
	int numGoods = -1;

	float increment_rate = 0.01f;

	vector<Agent> agents;

	EconomicSimulation(int numAgents, int numGoods) {
		this->numAgents = numAgents;
		this->numGoods = numGoods;

		for (int i = 0; i < numAgents; i++) {
			Agent new_agent(numAgents, numGoods, i);
			agents.push_back(new_agent);
		}
	}

	void simulationStep() {
		// INCOME AND WAGES

		// for each agent:
			// determine our income by summing together our market revenues
			// determine our wage rate from our income and labour committment

		for (Agent& agent : agents) {
			agent.income = 0.0f;
			for (int good = 0; good < numGoods; good++) {
				agent.income += agent.market_revenues[good];
			}
			agent.wage_rate = agent.income / agent.labour_commitment;
		}

		// SUPPLY AND DEMAND DETERMINE PRICE

		// for each agent:
			// for each good:
				// determine the labour allocated to the good
				// determine the supply of the good from the labour allocated

			// for each agent that spends in our market (including ourself):
				// add their spending to our market's revenue

			// for each good:
				// determine price of the good from market revenue and supply

		for (Agent& agent : agents) {
			for (int good = 0; good < numGoods; good++) {
				float labour = agent.labour_allocations[good] * agent.labour_commitment;
				float supply = agent.prod_functions[good].supply(labour);
				agent.supplies[good] = supply;
			}

			for (int good = 0; good < numGoods; good++) {
				agent.market_revenues[good] = 0.0f;
			}

			for (int good = 0; good < numGoods; good++) {
				for (Agent& other_agent : agents) {
					agent.market_revenues[good] += other_agent.income * agent.budget_allocations[agent.index][good];
				}
			}

			for (int good = 0; good < numGoods; good++) {
				agent.prices[good] = agent.market_revenues[good] / agent.supplies[good];
			}
		}

		// INDUSTRY IS REORGANIZED

		// for each agent:
			// for each good:
				// determine the good with the largest d(supply * price) / d(labour)

			// The good with the largest d(supply * price) / d(labour) will be allocated slightly more labour

		for (Agent& agent : agents) {
			agent.dVdL.clear();
			for (int good = 0; good < numGoods; good++) {
				float labour = agent.labour_allocations[good] * agent.labour_commitment;
				float dSdL = agent.prod_functions[good].dSdL(labour);
				float dVdL = dSdL * agent.prices[good];
				agent.dVdL.push_back(dVdL);
			}
			agent.most_labour_productive_good = max_element(agent.dVdL.begin(), agent.dVdL.end()) - agent.dVdL.begin();
			agent.labour_allocations[agent.most_labour_productive_good] *= (1.0f + increment_rate);
			float sum = 0.0f;
			for (float x : agent.labour_allocations) {
				sum += x;
			}
			for (float& alloc : agent.labour_allocations) {
				alloc /= sum;
			}
		}

		// THE AGENT INTROSPECTS ON HER CONSUMPTION

		// for each agent:
			// for each good:
				// for each agent:
					// add the amount of the good we expect to buy from the agent (including ourself) to a running total
					// the amount of the good we expect to buy should depend on the new prices set at the beginning of this step

				// determine the marginal utility of the good from our expected consumption

				// for each agent:
					// find the cheapest price for the good

				// divide the marginal utility by the cheapest price

			// for each good:
				// find the good with the largest (marginal utility) / (cheapest price)
				// hold onto this value for the next part: introspecting on work

			// commit to spending more of our budget on the good with the highest utility in the market with the cheapest price.

		for (Agent& agent : agents) {
			// get our level of actuall consumption from the prices markets cleared at this step
			agent.consumptions.clear();
			for (int good = 0; good < numGoods; good++) {
				float consumption = 0.0f;
				for (Agent& other_agent : agents) {
					consumption += (agent.income * agent.budget_allocations[other_agent.index][good]) / other_agent.prices[good];
				}
				agent.consumptions.push_back(consumption);
			}

			vector<float> marginal_utilities;
			for (int good = 0; good < numGoods; good++) {
				float marginal_utility = agent.marg_utilities[good].utility(agent.consumptions[good]);
				marginal_utilities.push_back(marginal_utility);
			}

			vector<float> cheapest_prices;
			vector<int> cheapest_agents;
			for (int good = 0; good < numGoods; good++) {
				float cheapest_price = FLT_MAX;
				int index_cheapest_agent = -1;
				for (Agent& other_agent : agents) {
					if (other_agent.prices[good] < cheapest_price) {
						cheapest_price = other_agent.prices[good];
						index_cheapest_agent = other_agent.index;
					}
				}
				cheapest_prices.push_back(cheapest_price);
				cheapest_agents.push_back(index_cheapest_agent);
			}

			vector<float> values;
			for (int good = 0; good < numGoods; good++) {
				float value = marginal_utilities[good] / cheapest_prices[good];
			}
		}

		// THE AGENT INTROSPECTS ON HER WORK

		// for each agent:
			// calculate (marginal disutility of labour) / (wage rate)
			// compare that value to the (marginal utility) / (cheapest price) found in the previous part
			// The agent will commit to working more or less depending on which value is larger

	}
};