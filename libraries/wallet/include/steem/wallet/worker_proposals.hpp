#pragma once

#include <vector>
#include <string>

#include <steem/plugins/condenser_api/condenser_api_legacy_asset.hpp>

namespace steem { namespace proposal {

        struct CreateProposal {

            static const int create_proposal_args_cnt = 7;

            /* Proposal creator */
            steem::protocol::account_name_type creator;

            /* Funds receiver */
            steem::protocol::account_name_type receiver;

            /* Start date */
            time_point_sec start_date;

            /* End date */
            time_point_sec end_date;

            /* Proposal daily payment */
            steem::plugins::condenser_api::legacy_asset daily_pay;

            /* Subject of proposal */
            std::string subject;

            /* Url with proposal description */
            std::string url;

            CreateProposal(steem::protocol::account_name_type _creator
                        , steem::protocol::account_name_type _receiver
                        , time_point_sec _start_date
                        , time_point_sec _end_date
                        , steem::plugins::condenser_api::legacy_asset _daily_pay
                        , const std::string& _subject
                        , const std::string& _url) : creator{_creator}
                                            , receiver{_receiver}
                                            , start_date{_start_date}
                                            , end_date{_end_date}
                                            , daily_pay{_daily_pay}
                                            , subject{_subject}
                                            , url{_url}
            {}
        };

        struct UpdateProposalVotes {
            
            static const int update_proposal_votes_args_cnt = 3;

            using Proposals = std::vector<int>;

            /* Proposal voter */
            steem::protocol::account_name_type voter;

            /* Voters proposals */
            Proposals proposals;

            /*Proposal(s) approval */
            bool approve;

            UpdateProposalVotes(steem::protocol::account_name_type& _voter
                            , Proposals _proposals
                            , bool _approve) : voter(_voter)
                                            , approve(_approve)
            {
                proposals = _proposals;
            }
        };
    }
}   //steem::proposals