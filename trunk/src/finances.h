/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file finances.h Definition of finances of the park. */

#ifndef FINANCES_H
#define FINANCES_H
#include "money.h"
#include "language.h"
#include "gamelevel.h"

static const int NUM_FINANCE_HISTORY = 4; ///< Number of finance objects to keep for history.


/**
 * Tracking monthly finances.
 * @ingroup finances_group
 */
class Finances {
public:
	Finances();
	~Finances();

	Money ride_construct; ///< Monthly expenditures for ride construction.
	Money ride_running;   ///< Monthly expenditures for ride running costs.
	Money land_purchase;  ///< Monthly expenditures for land purchase.
	Money landscaping;    ///< Monthly expenditures for landscaping.
	Money park_tickets;   ///< Monthly earnings for park tickets.
	Money ride_tickets;   ///< Monthly earnings for ride tickets.
	Money shop_sales;     ///< Monthly earnings for shop sales.
	Money shop_stock;     ///< Monthly expenditures for shop stock.
	Money food_sales;     ///< Monthly earnings for food sales.
	Money food_stock;     ///< Monthly expenditures for food stock.
	Money staff_wages;    ///< Monthly expenditures for staff wages.
	Money marketing;      ///< Monthly expenditures for marketing.
	Money research;       ///< Monthly expenditures for research.
	Money loan_interest;  ///< Monthly expenditures for loan interest.

	void Reset();
	Money GetTotal() const;
};


/**
 * A manager of finance objects.
 * @ingroup finances_group
 */
class FinancesManager {
protected:
	Finances finances[NUM_FINANCE_HISTORY]; ///< All finance objects needed for statistics.
	int num_used;                           ///< Number of %Finances objects that have history.
	int current;                            ///< Index for the current month's %Finances object.
	Money cash;                             ///< The user's current cash.

public:
	FinancesManager();
	~FinancesManager();

	const Finances &GetFinances();
	void AdvanceMonth();
	void CashToStrParams();
	void SetScenario(const Scenario &s);

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayRideConstruct(const Money &m) {
		this->finances[this->current].ride_construct -= m;
		this->cash -= m;
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayRideRunning(const Money &m) {
		this->finances[this->current].ride_running -= m;
		this->cash -= m;
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayLandPurchase(const Money &m) {
		this->finances[this->current].land_purchase -= m;
		this->cash -= m;
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayLandscaping(const Money &m) {
		this->finances[this->current].landscaping -= m;
		this->cash -= m;
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayShopStock(const Money &m) {
		this->finances[this->current].shop_stock -= m;
		this->cash -= m;
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayFoodStock(const Money &m) {
		this->finances[this->current].food_stock -= m;
		this->cash -= m;
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayStaffWages(const Money &m) {
		this->finances[this->current].staff_wages -= m;
		this->cash -= m;
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayMarketing(const Money &m) {
		this->finances[this->current].marketing -= m;
		this->cash -= m;
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayResearch(const Money &m) {
		this->finances[this->current].research -= m;
		this->cash -= m;
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayLoanInterest(const Money &m) {
		this->finances[this->current].loan_interest -= m;
		this->cash -= m;
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to earn.
	 */
	inline void EarnParkTickets(const Money &m) {
		this->finances[this->current].park_tickets += m;
		this->cash += m;
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to earn.
	 */
	inline void EarnRideTickets(const Money &m) {
		this->finances[this->current].ride_tickets += m;
		this->cash += m;
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to earn.
	 */
	inline void EarnShopSales(const Money &m) {
		this->finances[this->current].shop_sales += m;
		this->cash += m;
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to earn.
	 */
	inline void EarnFoodSales(const Money &m) {
		this->finances[this->current].food_sales += m;
		this->cash += m;
	}
};

extern FinancesManager _finances_manager;

#endif

